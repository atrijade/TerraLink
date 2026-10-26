#include <gtest/gtest.h>
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/orbit/satellite.h"

#include <cmath>
#include <algorithm>
#include <set>

using namespace NtnSim::Routing;
using namespace NtnSim::Orbit;

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
