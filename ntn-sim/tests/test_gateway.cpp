#include <gtest/gtest.h>
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/signal/connection.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/packet/packet.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/core/event_scheduler.h"

using namespace NtnSim;

class GatewayTest : public ::testing::Test {
protected:
    void SetUp() override {
        Utils::GeodeticPosition pos{37.7749, -122.4194, 10.0};
        m_gateway = std::make_unique<Gateway::Gateway>("gw-0", pos);
    }

    std::unique_ptr<Gateway::Gateway> m_gateway;
};

// =========================================================================
// Step 1: Feeder Link Profile Specifications & Satellite Visibility Tests
// =========================================================================

TEST_F(GatewayTest, FeederLinkProfile_KaBandDefaults) {
    auto profile = m_gateway->GetFeederLinkProfile();
    EXPECT_DOUBLE_EQ(profile.carrierFrequencyHz, 28.0e9); // 28 GHz Ka-band
    EXPECT_DOUBLE_EQ(profile.bandwidthHz, 500.0e6);       // 500 MHz
    EXPECT_DOUBLE_EQ(profile.dataRateBps, 2.5e9);         // 2.5 Gbps
    EXPECT_DOUBLE_EQ(profile.minElevationDeg, 10.0);      // 10.0 deg mask
    EXPECT_DOUBLE_EQ(profile.txPowerDbm, 43.0);
    EXPECT_DOUBLE_EQ(profile.antennaGainDbi, 45.0);
}

TEST_F(GatewayTest, FeederLinkProfile_QvBandPreset) {
    auto qv = Gateway::FeederLinkProfile::QvBand();
    EXPECT_DOUBLE_EQ(qv.carrierFrequencyHz, 40.0e9);      // 40 GHz Q/V-band
    EXPECT_DOUBLE_EQ(qv.bandwidthHz, 1.0e9);              // 1.0 GHz
    EXPECT_DOUBLE_EQ(qv.dataRateBps, 10.0e9);             // 10 Gbps
    EXPECT_DOUBLE_EQ(qv.minElevationDeg, 10.0);

    m_gateway->SetFeederLinkProfile(qv);
    EXPECT_DOUBLE_EQ(m_gateway->GetFeederLinkProfile().carrierFrequencyHz, 40.0e9);
    EXPECT_DOUBLE_EQ(m_gateway->GetFeederLinkProfile().dataRateBps, 10.0e9);
}

TEST_F(GatewayTest, ElevationMaskVisibilityCheck) {
    // Minimum elevation mask is 10.0 deg
    EXPECT_FALSE(m_gateway->IsSatelliteVisible(0.0));
    EXPECT_FALSE(m_gateway->IsSatelliteVisible(9.9));
    EXPECT_TRUE(m_gateway->IsSatelliteVisible(10.0));
    EXPECT_TRUE(m_gateway->IsSatelliteVisible(45.0));
    EXPECT_TRUE(m_gateway->IsSatelliteVisible(90.0));
}

TEST_F(GatewayTest, SatelliteConnectionLifecycle) {
    EXPECT_FALSE(m_gateway->HasActiveFeederLink());
    EXPECT_TRUE(m_gateway->GetConnectedSatelliteId().empty());

    m_gateway->ConnectSatellite("sat-p0-s1");
    EXPECT_TRUE(m_gateway->HasActiveFeederLink());
    EXPECT_EQ(m_gateway->GetConnectedSatelliteId(), "sat-p0-s1");

    m_gateway->DisconnectSatellite();
    EXPECT_FALSE(m_gateway->HasActiveFeederLink());
    EXPECT_TRUE(m_gateway->GetConnectedSatelliteId().empty());
}

// =========================================================================
// Step 2: Packet Sink & Statistics Tests
// =========================================================================

TEST_F(GatewayTest, InOrderPacketReception) {
    auto p1 = std::make_shared<Packet::Packet>(1, 1500, 0.0, "ue-1", "gw-0");
    auto p2 = std::make_shared<Packet::Packet>(2, 1500, 0.010, "ue-1", "gw-0");

    EXPECT_TRUE(m_gateway->ReceivePacket(p1, 0.020)); // latency = 20 ms
    EXPECT_TRUE(m_gateway->ReceivePacket(p2, 0.030)); // latency = 20 ms

    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 2u);
    EXPECT_EQ(m_gateway->GetTotalBytesReceived(), 3000u);
    EXPECT_EQ(m_gateway->GetOutOfOrderPackets(), 0u);
    EXPECT_EQ(m_gateway->GetLostPackets(), 0u);
    EXPECT_NEAR(m_gateway->GetAverageLatencySeconds(), 0.020, 1e-6);
}

TEST_F(GatewayTest, OutOfOrderPacketDetection) {
    auto p1 = std::make_shared<Packet::Packet>(10, 1500, 0.0, "ue-1", "gw-0");
    auto p2 = std::make_shared<Packet::Packet>(8, 1500, 0.005, "ue-1", "gw-0");

    m_gateway->ReceivePacket(p1, 0.020);
    m_gateway->ReceivePacket(p2, 0.025); // seq 8 arrived after seq 10

    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 2u);
    EXPECT_EQ(m_gateway->GetOutOfOrderPackets(), 1u);
}

TEST_F(GatewayTest, LostPacketSequenceGapDetection) {
    auto p1 = std::make_shared<Packet::Packet>(1, 1500, 0.0, "ue-1", "gw-0");
    auto p2 = std::make_shared<Packet::Packet>(5, 1500, 0.05, "ue-1", "gw-0"); // Gap: packets 2, 3, 4 lost

    m_gateway->ReceivePacket(p1, 0.020);
    m_gateway->ReceivePacket(p2, 0.070);

    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 2u);
    EXPECT_EQ(m_gateway->GetLostPackets(), 3u); // (5 - 1 - 1) = 3 lost
}

TEST_F(GatewayTest, InterArrivalJitterCalculation_RFC3550) {
    // Packet 1: Creation 0.000s, Arrival 0.020s -> Latency = 20 ms
    auto p1 = std::make_shared<Packet::Packet>(1, 1500, 0.000, "ue-1", "gw-0");
    m_gateway->ReceivePacket(p1, 0.020);
    EXPECT_DOUBLE_EQ(m_gateway->GetJitterSeconds(), 0.0);

    // Packet 2: Creation 0.010s, Arrival 0.046s -> Latency = 36 ms
    // Delay difference d = |36 ms - 20 ms| = 16 ms = 0.016s
    // RFC 3550: J = 0 + (0.016 - 0) / 16.0 = 0.001s = 1 ms
    auto p2 = std::make_shared<Packet::Packet>(2, 1500, 0.010, "ue-1", "gw-0");
    m_gateway->ReceivePacket(p2, 0.046);
    EXPECT_NEAR(m_gateway->GetJitterSeconds(), 0.001, 1e-6);

    // Packet 3: Creation 0.020s, Arrival 0.056s -> Latency = 36 ms
    // Delay difference d = |36 ms - 36 ms| = 0
    // RFC 3550: J = 0.001 + (0 - 0.001) / 16.0 = 0.001 * (15/16) = 0.0009375s
    auto p3 = std::make_shared<Packet::Packet>(3, 1500, 0.020, "ue-1", "gw-0");
    m_gateway->ReceivePacket(p3, 0.056);
    EXPECT_NEAR(m_gateway->GetJitterSeconds(), 0.0009375, 1e-6);
}

TEST_F(GatewayTest, GatewayThroughputTally) {
    for (uint64_t i = 1; i <= 1000; ++i) {
        double creation = (i - 1) * 0.001;
        double arrival = creation + 0.020;
        auto p = std::make_shared<Packet::Packet>(i, 1500, creation, "ue-1", "gw-0");
        m_gateway->ReceivePacket(p, arrival);
    }

    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 1000u);
    EXPECT_EQ(m_gateway->GetTotalBytesReceived(), 1500000u);
    EXPECT_NEAR(m_gateway->GetThroughputMbps(), 12.0, 0.1);
    EXPECT_GT(m_gateway->GetUtilizationPercent(), 0.0);
}

TEST_F(GatewayTest, PacketEngineDeliveryIntegration) {
    Packet::PacketEngine packetEngine;
    packetEngine.RegisterGateway(m_gateway.get());

    auto p = packetEngine.GeneratePacket(42, 1200, 0.100, "ue-1", "gw-0");
    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 0u);

    // DeliverPacket at destination gw-0
    packetEngine.DeliverPacket(p, 0.135); // 35 ms latency

    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 1u);
    EXPECT_EQ(m_gateway->GetTotalBytesReceived(), 1200u);
    EXPECT_NEAR(m_gateway->GetAverageLatencySeconds(), 0.035, 1e-6);
}

// =========================================================================
// Step 3 & 4: Feeder Handover, OrbitEngine, SignalEngine & RoutingEngine
// =========================================================================

TEST_F(GatewayTest, DishPointing_ZenithSatelliteGeometry) {
    // Place a satellite directly overhead at 600 km altitude
    Utils::GeodeticPosition zenithGeo{37.7749, -122.4194, 600000.0};
    Utils::Position3D zenithEcef = Utils::MathUtils::GeodeticToEcef(zenithGeo);

    Orbit::OrbitState state;
    state.SetEcefPosition(zenithEcef);
    state.SetEcefVelocity({0.0, 7.5, 0.0}); // 7.5 km/s velocity

    auto sat = std::make_shared<Orbit::Satellite>("sat-zenith", state);
    auto pointing = m_gateway->CalculateDishPointing(*sat);

    EXPECT_NEAR(pointing.elevationDeg, 90.0, 0.1);
    EXPECT_NEAR(pointing.slantRangeMeters, 600000.0, 100.0); // 600 km slant range
    EXPECT_TRUE(pointing.isVisible);
}

TEST_F(GatewayTest, DishPointing_DopplerShiftCalculation) {
    Utils::GeodeticPosition satGeo{38.5, -122.4194, 600000.0};
    Utils::Position3D satEcef = Utils::MathUtils::GeodeticToEcef(satGeo);

    Orbit::OrbitState state;
    state.SetEcefPosition(satEcef);
    // Approaching velocity: negative LoS component
    state.SetEcefVelocity({-5.0, 0.0, -5.0});

    auto sat = std::make_shared<Orbit::Satellite>("sat-doppler", state);
    auto pointing = m_gateway->CalculateDishPointing(*sat);

    EXPECT_NE(pointing.dopplerShiftHz, 0.0);
    EXPECT_NE(pointing.losVelocityKms, 0.0);
}

TEST_F(GatewayTest, CoverageEngine_GatewayOverloads) {
    Coverage::CoverageEngine coverageEngine;

    Utils::GeodeticPosition satGeo{37.7749, -122.4194, 600000.0};
    Orbit::OrbitState state;
    state.SetEcefPosition(Utils::MathUtils::GeodeticToEcef(satGeo));
    auto sat = std::make_shared<Orbit::Satellite>("sat-cov", state);

    auto info = coverageEngine.CalculateCoverage(*sat, *m_gateway);
    EXPECT_TRUE(info.IsVisible());
    EXPECT_NEAR(info.GetElevationAngleDeg(), 90.0, 0.5);

    auto visibleSats = coverageEngine.GetVisibleSatellites(*m_gateway, {sat});
    ASSERT_EQ(visibleSats.size(), 1u);
    EXPECT_EQ(visibleSats[0], "sat-cov");

    auto best = coverageEngine.GetBestSatellite(*m_gateway, {sat});
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->GetId(), "sat-cov");
}

TEST_F(GatewayTest, SignalEngine_FeederLinkBudget) {
    Signal::SignalEngine signalEngine;

    Utils::GeodeticPosition satGeo{37.7749, -122.4194, 600000.0};
    Orbit::OrbitState state;
    state.SetEcefPosition(Utils::MathUtils::GeodeticToEcef(satGeo));
    auto sat = std::make_shared<Orbit::Satellite>("sat-signal", state);

    auto sig = m_gateway->CalculateFeederSignal(*sat, &signalEngine);

    // At 28 GHz Ka-band, 600 km slant range:
    EXPECT_GT(sig.GetPathLossDb(), 170.0);
    EXPECT_LT(sig.GetLatencySeconds(), 0.01); // ~2 ms light travel time
    EXPECT_GT(sig.GetSinrDb(), 0.0);
}

TEST_F(GatewayTest, FeederHandover_ApproachingHorizonMaskAndRoutingUpdate) {
    Routing::RoutingEngine routingEngine;
    Metrics::MetricsEngine metricsEngine;

    // Sat A: low elevation near horizon (~11.3 deg, approaching 10 deg mask, below 12 deg handover threshold)
    Utils::GeodeticPosition satAGeo{37.7749 + 15.0, -122.4194, 600000.0};
    Orbit::OrbitState stateA;
    stateA.SetEcefPosition(Utils::MathUtils::GeodeticToEcef(satAGeo));
    auto satA = std::make_shared<Orbit::Satellite>("sat-A", stateA);

    // Sat B: rising satellite directly overhead (90 deg)
    Utils::GeodeticPosition satBGeo{37.7749, -122.4194, 600000.0};
    Orbit::OrbitState stateB;
    stateB.SetEcefPosition(Utils::MathUtils::GeodeticToEcef(satBGeo));
    auto satB = std::make_shared<Orbit::Satellite>("sat-B", stateB);

    // Initial connection to Sat A
    m_gateway->ConnectSatellite("sat-A");
    routingEngine.AddLink("gw-0", "sat-A", 1500000.0);
    EXPECT_TRUE(routingEngine.HasLink("gw-0", "sat-A"));
    EXPECT_FALSE(routingEngine.HasLink("gw-0", "sat-B"));

    // Evaluate feeder handover: threshold is 12 deg. Sat A has ~11 deg -> trigger handover to Sat B
    bool handedOver = m_gateway->EvaluateFeederHandover({satA, satB}, &routingEngine, 100.0, &metricsEngine, 12.0);

    EXPECT_TRUE(handedOver);
    EXPECT_EQ(m_gateway->GetConnectedSatelliteId(), "sat-B");
    EXPECT_TRUE(m_gateway->HasActiveFeederLink());

    // RoutingEngine graph was dynamically updated
    EXPECT_FALSE(routingEngine.HasLink("gw-0", "sat-A"));
    EXPECT_TRUE(routingEngine.HasLink("gw-0", "sat-B"));

    // Handover telemetry recorded in MetricsEngine
    auto handoverKPIs = metricsEngine.CalculateHandoverKPIs();
    EXPECT_EQ(handoverKPIs.totalAttempts, 1u);
    EXPECT_EQ(handoverKPIs.totalSuccesses, 1u);
}

TEST_F(GatewayTest, FeederHandover_SeamlessFailoverRerouting) {
    Routing::RoutingEngine routingEngine;

    // Setup network: UE -> sat-ISL -> sat-A (old feeder) -> gw-0
    //                              \-> sat-B (new feeder) -> gw-0
    routingEngine.AddLink("ue-1", "sat-ISL", 1000.0e3);
    routingEngine.AddLink("sat-ISL", "sat-A", 2000.0e3);
    routingEngine.AddLink("sat-ISL", "sat-B", 2000.0e3);
    routingEngine.AddLink("sat-A", "gw-0", 1500.0e3);

    m_gateway->ConnectSatellite("sat-A");

    // Route initially traverses sat-A
    auto initialRoute = routingEngine.CalculateRoute("ue-1", "gw-0");
    ASSERT_GE(initialRoute.size(), 3u);
    EXPECT_EQ(initialRoute[initialRoute.size() - 2], "sat-A");

    // Execute feeder handover from sat-A -> sat-B
    m_gateway->SwitchFeederSatellite("sat-B", 600.0e3, &routingEngine, 10.0);

    // Failover rerouting: packet route dynamically switches to sat-B without disruption
    auto failoverRoute = routingEngine.CalculateRoute("ue-1", "gw-0");
    ASSERT_GE(failoverRoute.size(), 3u);
    EXPECT_EQ(failoverRoute[failoverRoute.size() - 2], "sat-B");
}

TEST_F(GatewayTest, GatewayEngine_FleetManagementAndDI) {
    Gateway::GatewayEngine gatewayEngine;

    auto gw1 = std::make_shared<Gateway::Gateway>("gw-1", Utils::GeodeticPosition{40.7128, -74.0060, 10.0});
    auto gw2 = std::make_shared<Gateway::Gateway>("gw-2", Utils::GeodeticPosition{51.5074, -0.1278, 25.0});

    gatewayEngine.AddGateway(gw1);
    gatewayEngine.AddGateway(gw2);

    EXPECT_EQ(gatewayEngine.GetGatewayCount(), 2u);
    EXPECT_NE(gatewayEngine.GetGateway("gw-1"), nullptr);
    EXPECT_NE(gatewayEngine.GetGateway("gw-2"), nullptr);
    EXPECT_EQ(gatewayEngine.GetGateway("gw-nonexistent"), nullptr);

    EXPECT_TRUE(gatewayEngine.RemoveGateway("gw-1"));
    EXPECT_EQ(gatewayEngine.GetGatewayCount(), 1u);
}

TEST_F(GatewayTest, GatewayEngine_OrchestrationLoop) {
    Orbit::OrbitEngine orbitEngine;
    Coverage::CoverageEngine coverageEngine;
    Signal::SignalEngine signalEngine;
    Routing::RoutingEngine routingEngine;
    Packet::PacketEngine packetEngine;
    Metrics::MetricsEngine metricsEngine;

    Gateway::GatewayEngine gatewayEngine(&metricsEngine, &routingEngine);
    gatewayEngine.SetOrbitEngine(&orbitEngine);
    gatewayEngine.SetCoverageEngine(&coverageEngine);
    gatewayEngine.SetSignalEngine(&signalEngine);
    gatewayEngine.SetPacketEngine(&packetEngine);

    // Add satellite directly overhead
    Utils::GeodeticPosition satGeo{37.7749, -122.4194, 600000.0};
    Orbit::OrbitState state;
    state.SetEcefPosition(Utils::MathUtils::GeodeticToEcef(satGeo));
    orbitEngine.AddSatellite(std::make_shared<Orbit::Satellite>("sat-overhead", state));

    gatewayEngine.AddGateway(m_gateway.get());

    // Update feeder links orchestration
    gatewayEngine.UpdateFeederLinks(0.0);

    // Gateway automatically established feeder link to overhead satellite
    EXPECT_TRUE(m_gateway->HasActiveFeederLink());
    EXPECT_EQ(m_gateway->GetConnectedSatelliteId(), "sat-overhead");
    EXPECT_TRUE(routingEngine.HasLink("gw-0", "sat-overhead"));

    // Verify signal engine has the active feeder connection
    auto conn = signalEngine.GetConnection("gw-0", "sat-overhead");
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->IsActive());
}

TEST_F(GatewayTest, Gateway_InjectUplinkPacket) {
    Packet::PacketEngine packetEngine;
    Core::EventScheduler scheduler;

    auto packet = m_gateway->InjectUplinkPacket(packetEngine, 999, 1400, 1.5, "remote-ue", &scheduler);

    ASSERT_NE(packet, nullptr);
    EXPECT_EQ(packet->GetId(), 999u);
    EXPECT_EQ(packet->GetSourceNodeId(), "gw-0");
    EXPECT_EQ(packet->GetDestinationNodeId(), "remote-ue");
    EXPECT_EQ(packet->GetSizeBytes(), 1400u);
}
