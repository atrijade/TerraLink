#include <gtest/gtest.h>
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/packet/packet.h"
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"

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
    // Receive 1000 packets of 1500 bytes each = 1,500,000 bytes = 12,000,000 bits
    // Spread across 1.0 second: from t=0.0 to t=1.0s
    // Throughput should equal 12.0 Mbps
    for (uint64_t i = 1; i <= 1000; ++i) {
        double creation = (i - 1) * 0.001;
        double arrival = creation + 0.020;
        auto p = std::make_shared<Packet::Packet>(i, 1500, creation, "ue-1", "gw-0");
        m_gateway->ReceivePacket(p, arrival);
    }

    EXPECT_EQ(m_gateway->GetTotalPacketsReceived(), 1000u);
    EXPECT_EQ(m_gateway->GetTotalBytesReceived(), 1500000u);
    // Duration: 1.019s - 0.020s = 0.999s
    // Throughput = (1,500,000 * 8) / (0.999 * 1e6) = 12.012 Mbps
    EXPECT_NEAR(m_gateway->GetThroughputMbps(), 12.0, 0.1);
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
