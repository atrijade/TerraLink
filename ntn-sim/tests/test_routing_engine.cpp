#include <gtest/gtest.h>
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/core/event_scheduler.h"

#include <cmath>
#include <algorithm>
#include <set>

using namespace NtnSim::Routing;
using namespace NtnSim::Orbit;
using namespace NtnSim::Packet;
using namespace NtnSim::Metrics;
using namespace NtnSim::Core;

// ---------------------------------------------------------------------------
// Test 1: Skeleton Fallback (Backwards compatibility)
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, ResolveNextHopSkeleton) {
    RoutingEngine engine;
    
    // When topology is uninitialized, engine returns src and dest as fallback
    std::vector<std::string> route = engine.CalculateRoute("sat-1", "gw-0");
    ASSERT_EQ(route.size(), 2);
    EXPECT_EQ(route.front(), "sat-1");
    EXPECT_EQ(route.back(), "gw-0");
    
    std::string nextHop = engine.ResolveNextHop("sat-1", "gw-0");
    EXPECT_EQ(nextHop, "gw-0");
}

// ---------------------------------------------------------------------------
// Test 2: Phase 1 — Constellation Topology & 4-ISL Mesh Generation
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, ConstellationMeshTopology4ISL) {
    // Generate Walker Delta 66/6/1 constellation (P = 6 planes, S = 11 sats/plane = 66 sats)
    OrbitEngine orbitEngine;
    WalkerConfig config;
    config.planes = 6;
    config.satsPerPlane = 11;
    config.altitudeKm = 600.0;
    config.inclinationDeg = 70.0; // sub-polar so links start active
    config.phasingParameter = 1;
    orbitEngine.GenerateWalkerConstellation(config);

    auto satellites = orbitEngine.GetAllSatellites();
    ASSERT_EQ(satellites.size(), 66);

    RoutingEngine routingEngine;
    routingEngine.BuildConstellationTopology(satellites, 6, 11, 75.0);

    // Verify all 66 satellite nodes are registered
    auto nodes = routingEngine.GetAllNodes();
    EXPECT_EQ(nodes.size(), 66);

    // Verify satellite sat-p2-s5 has exactly 4 ISL links
    std::string testSatId = "sat-p2-s5";
    auto links = routingEngine.GetLinksForNode(testSatId);
    ASSERT_EQ(links.size(), 4);

    // Collect neighbor IDs
    std::set<std::string> neighbors;
    int intraCount = 0;
    int interCount = 0;

    for (const auto& link : links) {
        neighbors.insert(link.neighborSatId);
        if (link.isInterPlane) {
            interCount++;
        } else {
            intraCount++;
        }
        EXPECT_GT(link.distanceMeters, 1000.0); // > 1 km
        EXPECT_GT(link.propagationDelaySeconds, 0.0);
        EXPECT_DOUBLE_EQ(link.dataRateBps, 10.0e9); // 10 Gbps laser link
    }

    // 2 Intra-plane: front (p2, s6), back (p2, s4)
    EXPECT_EQ(intraCount, 2);
    EXPECT_TRUE(neighbors.contains("sat-p2-s6"));
    EXPECT_TRUE(neighbors.contains("sat-p2-s4"));

    // 2 Inter-plane: right (p3, s5), left (p1, s5)
    EXPECT_EQ(interCount, 2);
    EXPECT_TRUE(neighbors.contains("sat-p3-s5"));
    EXPECT_TRUE(neighbors.contains("sat-p1-s5"));

    // Verify boundary wrapping: sat-p0-s0
    auto boundaryLinks = routingEngine.GetLinksForNode("sat-p0-s0");
    ASSERT_EQ(boundaryLinks.size(), 4);
    std::set<std::string> bNeighbors;
    for (const auto& l : boundaryLinks) {
        bNeighbors.insert(l.neighborSatId);
    }
    // Front: s1, Back: s10 (modulo 11)
    EXPECT_TRUE(bNeighbors.contains("sat-p0-s1"));
    EXPECT_TRUE(bNeighbors.contains("sat-p0-s10"));
    // Right: p1, Left: p5 (modulo 6)
    EXPECT_TRUE(bNeighbors.contains("sat-p1-s0"));
    EXPECT_TRUE(bNeighbors.contains("sat-p5-s0"));
}

// ---------------------------------------------------------------------------
// Test 3: Phase 1 — Polar Latitude Shut-off Logic (lat > 70 deg)
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, PolarLatitudeShutoff) {
    OrbitEngine orbitEngine;
    // Satellites with 87.5 deg inclination (polar orbit)
    WalkerConfig config;
    config.planes = 4;
    config.satsPerPlane = 6;
    config.altitudeKm = 600.0;
    config.inclinationDeg = 87.5;
    orbitEngine.GenerateWalkerConstellation(config);

    auto satellites = orbitEngine.GetAllSatellites();
    ASSERT_EQ(satellites.size(), 24);

    // Propagate so satellites move along orbit
    orbitEngine.UpdatePositions(100.0);

    RoutingEngine routingEngine;
    // 70.0 deg polar shut-off threshold
    routingEngine.BuildConstellationTopology(satellites, 4, 6, 70.0);

    // Check links on every satellite
    for (const auto& sat : satellites) {
        auto ecef = sat->GetOrbitState().GetEcefPosition();
        double r = std::sqrt(ecef.x * ecef.x + ecef.y * ecef.y + ecef.z * ecef.z);
        double latDeg = std::asin(std::clamp(ecef.z / r, -1.0, 1.0)) * 180.0 / std::numbers::pi;

        auto links = routingEngine.GetLinksForNode(sat->GetId());
        for (const auto& link : links) {
            if (!link.isInterPlane) {
                // Intra-plane links must ALWAYS remain active regardless of latitude
                EXPECT_TRUE(link.isActive);
            } else {
                // If this satellite is above 70 deg latitude, inter-plane links must be deactivated
                if (std::abs(latDeg) > 70.0) {
                    EXPECT_FALSE(link.isActive);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Test 4: Phase 1 — Dynamic UpdateTopology() with orbital propagation
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, UpdateTopologyDynamicPropagation) {
    OrbitEngine orbitEngine;
    WalkerConfig config;
    config.planes = 3;
    config.satsPerPlane = 4;
    config.altitudeKm = 600.0;
    config.inclinationDeg = 60.0;
    orbitEngine.GenerateWalkerConstellation(config);

    auto satellites = orbitEngine.GetAllSatellites();
    RoutingEngine routingEngine;
    routingEngine.BuildConstellationTopology(satellites, 3, 4, 70.0);

    auto linkBefore = routingEngine.GetLink("sat-p0-s0", "sat-p1-s0");
    ASSERT_TRUE(linkBefore.has_value());
    double distBefore = linkBefore->distanceMeters;

    // Advance orbital time by 1200 seconds (~20 minutes)
    orbitEngine.UpdatePositions(1200.0);
    routingEngine.UpdateTopology();

    auto linkAfter = routingEngine.GetLink("sat-p0-s0", "sat-p1-s0");
    ASSERT_TRUE(linkAfter.has_value());
    double distAfter = linkAfter->distanceMeters;

    // Due to orbital motion and Earth rotation, Euclidean distance between planes changes
    EXPECT_NE(distBefore, distAfter);
    EXPECT_DOUBLE_EQ(linkAfter->propagationDelaySeconds, distAfter / RoutingEngine::SPEED_OF_LIGHT_M_S);
}

// ---------------------------------------------------------------------------
// Test 5: Phase 2 — Dijkstra Shortest-Path (MinLatency Metric)
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, DijkstraShortestPathMinLatency) {
    RoutingEngine routingEngine;
    routingEngine.SetMetricType(RoutingMetric::MinLatency);

    // Create a diamond topology:
    //         (Node-B) [dist = 1,000 km]
    //       /          |
    // (Node-A)          (Node-D)
    //       \          /
    //         (Node-C) [dist = 5,000 km]
    double d1 = 1000.0e3; // 1,000 km
    double d2 = 5000.0e3; // 5,000 km

    routingEngine.AddLink("Node-A", "Node-B", d1);
    routingEngine.AddLink("Node-B", "Node-D", d1);
    routingEngine.AddLink("Node-A", "Node-C", d2);
    routingEngine.AddLink("Node-C", "Node-D", d2);

    auto route = routingEngine.CalculateRoute("Node-A", "Node-D");
    ASSERT_EQ(route.size(), 3);
    EXPECT_EQ(route[0], "Node-A");
    EXPECT_EQ(route[1], "Node-B");
    EXPECT_EQ(route[2], "Node-D");

    double expectedLatency = (2.0 * d1) / RoutingEngine::SPEED_OF_LIGHT_M_S;
    EXPECT_NEAR(routingEngine.CalculateRouteLatency(route), expectedLatency, 1.0e-6);
}

// ---------------------------------------------------------------------------
// Test 6: Phase 2 — Dijkstra Shortest-Path (MinHops Metric)
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, DijkstraShortestPathMinHops) {
    RoutingEngine routingEngine;
    routingEngine.SetMetricType(RoutingMetric::MinHops);

    // Path 1 (Fewer hops, but higher physical distance):
    // Node-A -> Node-X -> Node-D (2 hops, 10,000 km total)
    routingEngine.AddLink("Node-A", "Node-X", 5000.0e3);
    routingEngine.AddLink("Node-X", "Node-D", 5000.0e3);

    // Path 2 (More hops, but lower physical distance):
    // Node-A -> Node-1 -> Node-2 -> Node-3 -> Node-D (4 hops, 2,000 km total)
    routingEngine.AddLink("Node-A", "Node-1", 500.0e3);
    routingEngine.AddLink("Node-1", "Node-2", 500.0e3);
    routingEngine.AddLink("Node-2", "Node-3", 500.0e3);
    routingEngine.AddLink("Node-3", "Node-D", 500.0e3);

    // MinHops must select the 2-hop route (Node-A -> Node-X -> Node-D)
    auto route = routingEngine.CalculateRoute("Node-A", "Node-D");
    ASSERT_EQ(route.size(), 3);
    EXPECT_EQ(route[0], "Node-A");
    EXPECT_EQ(route[1], "Node-X");
    EXPECT_EQ(route[2], "Node-D");
    EXPECT_DOUBLE_EQ(routingEngine.CalculateRouteCost(route), 2.0); // 2 hops
}

// ---------------------------------------------------------------------------
// Test 7: Phase 2 — Congestion-Aware Routing (Deflection via T_queue)
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, CongestionAwareRoutingDeflection) {
    RoutingEngine routingEngine;
    routingEngine.SetMetricType(RoutingMetric::CongestionAware);

    // Setup 2 equal physical length paths:
    // Path 1: Node-A -> Node-Fast -> Node-D (1,000 km per hop)
    // Path 2: Node-A -> Node-Alt  -> Node-D (1,100 km per hop)
    routingEngine.AddLink("Node-A", "Node-Fast", 1000.0e3);
    routingEngine.AddLink("Node-Fast", "Node-D", 1000.0e3);

    routingEngine.AddLink("Node-A", "Node-Alt", 1100.0e3);
    routingEngine.AddLink("Node-Alt", "Node-D", 1100.0e3);

    // Initially with 0 queue delay, Fast path is chosen
    auto initialRoute = routingEngine.CalculateRoute("Node-A", "Node-D");
    ASSERT_EQ(initialRoute.size(), 3);
    EXPECT_EQ(initialRoute[1], "Node-Fast");

    // Introduce large buffer congestion at Node-Fast (e.g. 50 ms queuing delay)
    routingEngine.SetNodeQueuingDelay("Node-Fast", 0.050); // 50 ms

    // CongestionAware routing should dynamically divert through Node-Alt!
    auto divertedRoute = routingEngine.CalculateRoute("Node-A", "Node-D");
    ASSERT_EQ(divertedRoute.size(), 3);
    EXPECT_EQ(divertedRoute[0], "Node-A");
    EXPECT_EQ(divertedRoute[1], "Node-Alt");
    EXPECT_EQ(divertedRoute[2], "Node-D");
}

// ---------------------------------------------------------------------------
// Test 8: Phase 2 — ResolveNextHop and Forwarding Table Caching
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, ResolveNextHopAndForwardingLookup) {
    RoutingEngine routingEngine;
    routingEngine.SetMetricType(RoutingMetric::MinLatency);

    // Linear chain: Sat-0 -> Sat-1 -> Sat-2 -> Sat-3
    routingEngine.AddLink("Sat-0", "Sat-1", 1000.0e3);
    routingEngine.AddLink("Sat-1", "Sat-2", 1000.0e3);
    routingEngine.AddLink("Sat-2", "Sat-3", 1000.0e3);

    // Immediate next hop from Sat-0 towards Sat-3 is Sat-1
    std::string nextHop0 = routingEngine.ResolveNextHop("Sat-0", "Sat-3");
    EXPECT_EQ(nextHop0, "Sat-1");

    // Forwarding lookup at Sat-1 towards Sat-3 is Sat-2
    std::string nextHop1 = routingEngine.ResolveNextHop("Sat-1", "Sat-3");
    EXPECT_EQ(nextHop1, "Sat-2");

    // Forwarding lookup at Sat-2 towards Sat-3 is Sat-3
    std::string nextHop2 = routingEngine.ResolveNextHop("Sat-2", "Sat-3");
    EXPECT_EQ(nextHop2, "Sat-3");

    // At destination, returns current node
    std::string atDest = routingEngine.ResolveNextHop("Sat-3", "Sat-3");
    EXPECT_EQ(atDest, "Sat-3");
}

// ---------------------------------------------------------------------------
// Test 9: Dynamic Link Failover & Rerouting
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, DynamicLinkFailover) {
    RoutingEngine routingEngine;
    routingEngine.SetMetricType(RoutingMetric::MinLatency);

    // Triangle: Node-A <-> Node-B <-> Node-C, plus direct Node-A <-> Node-C
    routingEngine.AddLink("Node-A", "Node-C", 1000.0e3); // Direct shortest link
    routingEngine.AddLink("Node-A", "Node-B", 600.0e3);
    routingEngine.AddLink("Node-B", "Node-C", 600.0e3);

    auto route1 = routingEngine.CalculateRoute("Node-A", "Node-C");
    ASSERT_EQ(route1.size(), 2);
    EXPECT_EQ(route1[0], "Node-A");
    EXPECT_EQ(route1[1], "Node-C");

    // Break the direct link (e.g. laser hardware fault)
    routingEngine.SetLinkActive("Node-A", "Node-C", false);

    // Dijkstra should detect broken link and route around via Node-B
    auto route2 = routingEngine.CalculateRoute("Node-A", "Node-C");
    ASSERT_EQ(route2.size(), 3);
    EXPECT_EQ(route2[0], "Node-A");
    EXPECT_EQ(route2[1], "Node-B");
    EXPECT_EQ(route2[2], "Node-C");
}

// ---------------------------------------------------------------------------
// Test 10: End-to-End Route with Ground UE and Gateway
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, EndToEndRouteWithUEAndGateway) {
    RoutingEngine routingEngine;
    routingEngine.SetMetricType(RoutingMetric::MinLatency);

    // Feeder uplink: ue-1 -> sat-p0-s0
    routingEngine.AddLink("ue-1", "sat-p0-s0", 800.0e3);

    // Constellation ISL mesh: sat-p0-s0 -> sat-p0-s1 -> sat-p1-s1
    routingEngine.AddLink("sat-p0-s0", "sat-p0-s1", 2500.0e3);
    routingEngine.AddLink("sat-p0-s1", "sat-p1-s1", 3000.0e3, true);

    // Feeder downlink: sat-p1-s1 -> gw-0
    routingEngine.AddLink("sat-p1-s1", "gw-0", 750.0e3);

    auto route = routingEngine.CalculateRoute("ue-1", "gw-0");
    ASSERT_EQ(route.size(), 5);
    EXPECT_EQ(route[0], "ue-1");
    EXPECT_EQ(route[1], "sat-p0-s0");
    EXPECT_EQ(route[2], "sat-p0-s1");
    EXPECT_EQ(route[3], "sat-p1-s1");
    EXPECT_EQ(route[4], "gw-0");

    EXPECT_NEAR(routingEngine.CalculateRouteDistance(route), 7050.0e3, 1.0);
    EXPECT_EQ(routingEngine.ResolveNextHop("ue-1", "gw-0"), "sat-p0-s0");
    EXPECT_EQ(routingEngine.ResolveNextHop("sat-p0-s0", "gw-0"), "sat-p0-s1");
    EXPECT_EQ(routingEngine.ResolveNextHop("sat-p1-s1", "gw-0"), "gw-0");
}

// ---------------------------------------------------------------------------
// Test 11: Disconnected Components Return Empty
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, DisconnectedGraphReturnsEmpty) {
    RoutingEngine routingEngine;
    routingEngine.AddLink("Node-A", "Node-B", 1000.0e3);
    routingEngine.AddLink("Island-1", "Island-2", 1000.0e3);

    auto route = routingEngine.CalculateRoute("Node-A", "Island-2");
    EXPECT_TRUE(route.empty());

    std::string nextHop = routingEngine.ResolveNextHop("Node-A", "Island-2");
    EXPECT_TRUE(nextHop.empty());
}

// ---------------------------------------------------------------------------
// Test 12: Phase 3 — End-to-End Hop Loop & Multi-Hop Discrete-Event Forwarding
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, Phase3_EndToEndHopLoopIntegration) {
    auto metricsEngine = std::make_shared<MetricsEngine>();
    auto routingEngine = std::make_shared<RoutingEngine>();
    PacketEngine packetEngine(metricsEngine, routingEngine);
    EventScheduler scheduler;

    // Topology: user -> sat-1 -> sat-2 -> gw-0
    // Uplink: user -> sat-1 (600 km, 100 Mbps)
    routingEngine->AddLink("user", "sat-1", 600.0e3, false, 100.0e6);
    // Optical ISL: sat-1 -> sat-2 (2,000 km, 10 Gbps laser cross-link)
    routingEngine->AddLink("sat-1", "sat-2", 2000.0e3, false, 10.0e9);
    // Downlink: sat-2 -> gw-0 (600 km, 1 Gbps)
    routingEngine->AddLink("sat-2", "gw-0", 600.0e3, false, 1.0e9);

    // User spawns packet at t = 0.0 s destined for Gateway gw-0
    auto packet = packetEngine.GeneratePacket(101, 1500, 0.0, "user", "gw-0", 2, 64, 2.0);
    ASSERT_NE(packet, nullptr);
    EXPECT_DOUBLE_EQ(packet->GetCreationTime(), 0.0);

    // User uplinks packet to serving satellite -> RoutePacketMultiHop(user -> sat-1)
    bool started = packetEngine.RoutePacketMultiHop(packet, "user", 0.0, scheduler);
    EXPECT_TRUE(started);

    // Process all events in discrete-event scheduler (hop loop execution)
    size_t eventCount = 0;
    while (!scheduler.IsEmpty()) {
        scheduler.ProcessNextEvent();
        eventCount++;
    }

    // Must have executed intermediate hops + final PacketDelivered event
    EXPECT_GT(eventCount, 2u);

    // Verify packet reached destination gw-0
    EXPECT_EQ(packetEngine.GetTotalPacketsDelivered(), 1u);
    EXPECT_EQ(packet->GetHopCount(), 3u); // 3 hops across 4 nodes: ["user", "sat-1", "sat-2", "gw-0"]
    auto hops = packet->GetHopHistory();
    ASSERT_EQ(hops.size(), 4u);
    EXPECT_EQ(hops[0], "user");
    EXPECT_EQ(hops[1], "sat-1");
    EXPECT_EQ(hops[2], "sat-2");
    EXPECT_EQ(hops[3], "gw-0");

    // Metrics Engine Hook: Measures accumulated end-to-end latency across all space hops
    EXPECT_GT(packet->GetArrivalTime(), 0.0);
    EXPECT_DOUBLE_EQ(packet->GetEndToEndLatency(), packet->GetArrivalTime() - packet->GetCreationTime());

    auto kpis = metricsEngine->CalculateTrafficKPIs();
    EXPECT_EQ(kpis.packetsSent, 1u);
    EXPECT_EQ(kpis.packetsReceived, 1u);
    EXPECT_DOUBLE_EQ(kpis.packetDeliveryRatioPercent, 100.0);
    EXPECT_GT(kpis.meanLatencySeconds, 0.0);
}

// ---------------------------------------------------------------------------
// Test 13: Phase 3 — Multi-Hop Constellation Flight with 72-Sat Walker Delta
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, Phase3_WalkerDeltaMultiHopFlight) {
    auto metricsEngine = std::make_shared<MetricsEngine>();
    auto routingEngine = std::make_shared<RoutingEngine>();
    PacketEngine packetEngine(metricsEngine, routingEngine);
    EventScheduler scheduler;

    OrbitEngine orbitEngine;
    orbitEngine.GenerateWalkerConstellation(6, 12, 600.0, 53.0); // Starlink-like 53 deg Walker Delta
    auto satellites = orbitEngine.GetAllSatellites();
    routingEngine->BuildConstellationTopology(satellites, 6, 12, 70.0);

    // Attach ground UE to sat-p0-s0, and Gateway gw-0 to sat-p2-s3
    routingEngine->AddLink("ue-alpha", "sat-p0-s0", 700.0e3, false, 100.0e6);
    routingEngine->AddLink("sat-p2-s3", "gw-0", 650.0e3, false, 1.0e9);

    auto pkt = packetEngine.GeneratePacket(202, 1500, 0.0, "ue-alpha", "gw-0");
    bool routed = packetEngine.RoutePacketMultiHop(pkt, "ue-alpha", 0.0, scheduler);
    EXPECT_TRUE(routed);

    // Run discrete event loop to completion
    while (!scheduler.IsEmpty()) {
        scheduler.ProcessNextEvent();
    }

    EXPECT_EQ(packetEngine.GetTotalPacketsDelivered(), 1u);
    EXPECT_EQ(pkt->GetHopHistory().front(), "ue-alpha");
    EXPECT_EQ(pkt->GetHopHistory().back(), "gw-0");
    EXPECT_GT(pkt->GetHopCount(), 3u); // Traversed several constellation ISLs
    EXPECT_GT(pkt->GetEndToEndLatency(), 0.0);
}

// ---------------------------------------------------------------------------
// Test 14: Phase 4 — Dynamic Re-Routing & Link Failure (Failover)
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, Phase4_DynamicFailoverAndDetourRerouting) {
    auto metricsEngine = std::make_shared<MetricsEngine>();
    auto routingEngine = std::make_shared<RoutingEngine>();
    PacketEngine packetEngine(metricsEngine, routingEngine);
    EventScheduler scheduler;

    // Topology:
    // User uplinks to sat-1
    routingEngine->AddLink("user", "sat-1", 600.0e3, false, 100.0e6);

    // Primary optical ISL: sat-1 <-> sat-2 (direct shortest path, 1,000 km)
    routingEngine->AddLink("sat-1", "sat-2", 1000.0e3, false, 10.0e9);

    // Secondary detour path: sat-1 <-> sat-detour <-> sat-2 (1,500 km each)
    routingEngine->AddLink("sat-1", "sat-detour", 1500.0e3, false, 10.0e9);
    routingEngine->AddLink("sat-detour", "sat-2", 1500.0e3, false, 10.0e9);

    // Downlink: sat-2 -> gw-0 (600 km, 1 Gbps)
    routingEngine->AddLink("sat-2", "gw-0", 600.0e3, false, 1.0e9);

    // Before failure, route goes through sat-1 -> sat-2 -> gw-0
    auto primaryRoute = routingEngine->CalculateRoute("user", "gw-0");
    ASSERT_EQ(primaryRoute.size(), 4u);
    EXPECT_EQ(primaryRoute[1], "sat-1");
    EXPECT_EQ(primaryRoute[2], "sat-2");
    EXPECT_EQ(primaryRoute[3], "gw-0");

    // Phase 4: ISL link failure occurs (e.g. laser tracker fault)
    routingEngine->TriggerLinkFailure("sat-1", "sat-2");

    // Spawns packet at t = 0.0 s
    auto packet = packetEngine.GeneratePacket(303, 1500, 0.0, "user", "gw-0");
    bool ok = packetEngine.RoutePacketMultiHop(packet, "user", 0.0, scheduler);
    EXPECT_TRUE(ok);

    // Process event pipeline
    while (!scheduler.IsEmpty()) {
        scheduler.ProcessNextEvent();
    }

    // Packet must have successfully delivered via the detour!
    EXPECT_EQ(packetEngine.GetTotalPacketsDelivered(), 1u);
    auto history = packet->GetHopHistory();
    ASSERT_EQ(history.size(), 5u);
    EXPECT_EQ(history[0], "user");
    EXPECT_EQ(history[1], "sat-1");
    EXPECT_EQ(history[2], "sat-detour"); // Successfully detoured around severed link!
    EXPECT_EQ(history[3], "sat-2");
    EXPECT_EQ(history[4], "gw-0");

    // Restore link and verify routing returns to primary link
    routingEngine->RestoreLink("sat-1", "sat-2");
    auto restoredRoute = routingEngine->CalculateRoute("user", "gw-0");
    ASSERT_EQ(restoredRoute.size(), 4u);
    EXPECT_EQ(restoredRoute[2], "sat-2"); // Back to direct link
}

// ---------------------------------------------------------------------------
// Test 15: Phase 4 — Polar Shut-Off Link Disruption
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, Phase4_PolarShutOffLinkDisruption) {
    auto routingEngine = std::make_shared<RoutingEngine>();
    OrbitEngine orbitEngine;

    // Satellites with 90 deg (polar) inclination
    WalkerConfig config;
    config.planes = 4;
    config.satsPerPlane = 6;
    config.altitudeKm = 600.0;
    config.inclinationDeg = 90.0;
    orbitEngine.GenerateWalkerConstellation(config);

    auto satellites = orbitEngine.GetAllSatellites();
    // 70.0 deg polar shut-off threshold
    routingEngine->BuildConstellationTopology(satellites, 4, 6, 70.0);

    // Satellite 0 in Plane 0: sat-p0-s0
    // Check its cross-plane links before and after polar transit
    auto initialLink = routingEngine->GetLink("sat-p0-s0", "sat-p1-s0");
    ASSERT_TRUE(initialLink.has_value());

    // Propagate constellation into polar latitudes
    orbitEngine.UpdatePositions(1450.0); // Moves orbital phase towards 90 deg latitude
    routingEngine->UpdateTopology();

    // Verify polar shut-off state tracking
    bool hasAnyPolarSat = false;
    for (const auto& sat : satellites) {
        if (routingEngine->IsSatelliteInPolarRegion(sat->GetId())) {
            hasAnyPolarSat = true;
            // Verify all inter-plane links on this polar satellite are shut off
            auto nodeTopo = routingEngine->GetNodeTopology(sat->GetId());
            for (const auto& l : nodeTopo.activeLinks) {
                // Active links must ONLY be intra-plane (ring) links
                EXPECT_FALSE(l.isInterPlane);
            }
        }
    }
    EXPECT_TRUE(hasAnyPolarSat);
}

// ---------------------------------------------------------------------------
// Test 16: Phase 4 — Network Partition Drops Packet with NoRoute
// ---------------------------------------------------------------------------
TEST(RoutingEngineTest, Phase4_NetworkPartitionDropsPacketWithNoRoute) {
    auto metricsEngine = std::make_shared<MetricsEngine>();
    auto routingEngine = std::make_shared<RoutingEngine>();
    PacketEngine packetEngine(metricsEngine, routingEngine);
    EventScheduler scheduler;

    // Disconnected topology: user -> sat-1, and separated island sat-2 -> gw-0
    routingEngine->AddLink("user", "sat-1", 600.0e3, false, 100.0e6);
    routingEngine->AddLink("sat-2", "gw-0", 600.0e3, false, 1.0e9);

    auto packet = packetEngine.GeneratePacket(404, 1500, 0.0, "user", "gw-0");
    // Attempting to route across partitioned network
    bool scheduled = packetEngine.RoutePacketMultiHop(packet, "user", 0.0, scheduler);
    EXPECT_FALSE(scheduled); // Should immediately fail and drop

    // Verify packet dropped with NoRoute reason
    EXPECT_EQ(packetEngine.GetTotalPacketsDelivered(), 0u);
    EXPECT_EQ(packetEngine.GetTotalPacketsDropped(), 1u);

    auto kpis = metricsEngine->CalculateTrafficKPIs();
    EXPECT_EQ(kpis.packetsDropped, 1u);
    EXPECT_EQ(kpis.dropsNoRoute, 1u);
}
