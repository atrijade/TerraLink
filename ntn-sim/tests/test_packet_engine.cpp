#include <gtest/gtest.h>
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/packet/packet.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/signal/signal_info.h"
#include <cmath>
#include <vector>

TEST(PacketEngineTest, EnqueueAndDequeuePackets) {
    NtnSim::Packet::PacketEngine engine;
    
    auto pkt = engine.GeneratePacket(1001, 1500, 0.0, "user-1", "gw-0");
    EXPECT_EQ(pkt->GetId(), 1001u);
    EXPECT_EQ(pkt->GetSizeBytes(), 1500u);
    EXPECT_EQ(pkt->GetSourceNodeId(), "user-1");
    
    engine.EnqueuePacket("buffer-1", pkt);
    EXPECT_EQ(engine.GetQueueSize("buffer-1"), 1u);
    
    auto popped = engine.DequeuePacket("buffer-1");
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(popped->GetId(), 1001u);
    EXPECT_EQ(engine.GetQueueSize("buffer-1"), 0u);
}

TEST(PacketEngineTest, PacketHeaderMetadataAndTtlExpiry) {
    NtnSim::Packet::PacketEngine engine;

    // Packet with ID 2001, 1000 bytes, created at t=1.0s, qos=1 (voice), ttl=3, maxLifetime=2.0s
    auto pkt = engine.GeneratePacket(2001, 1000, 1.0, "user-10", "gw-0", 1, 3, 2.0);

    EXPECT_EQ(pkt->GetQosClass(), 1u);
    EXPECT_EQ(pkt->GetTtl(), 3u);
    EXPECT_DOUBLE_EQ(pkt->GetMaxLifetimeSeconds(), 2.0);

    // Initial hop history contains the source node
    EXPECT_EQ(pkt->GetHopCount(), 0u);
    EXPECT_EQ(pkt->GetHopHistory().front(), "user-10");

    // Trace route hops: user -> sat-0 -> sat-1 -> gw-0
    pkt->AddHop("sat-p0-s0");
    pkt->AddHop("sat-p0-s1");
    pkt->AddHop("gw-0");
    EXPECT_EQ(pkt->GetHopCount(), 3u);
    EXPECT_EQ(pkt->GetHopHistory().size(), 4u);

    // Test TTL decrement
    EXPECT_TRUE(pkt->DecrementTtl()); // 3 -> 2
    EXPECT_EQ(pkt->GetTtl(), 2u);
    EXPECT_TRUE(pkt->DecrementTtl()); // 2 -> 1
    EXPECT_EQ(pkt->GetTtl(), 1u);
    EXPECT_FALSE(pkt->DecrementTtl()); // 1 -> 0
    EXPECT_EQ(pkt->GetTtl(), 0u);

    // Expired due to TTL == 0
    EXPECT_TRUE(pkt->IsExpired(1.5));

    // Test lifetime timeout expiry
    auto freshPkt = engine.GeneratePacket(2002, 500, 10.0, "u1", "gw1", 2, 64, 2.0);
    EXPECT_FALSE(freshPkt->IsExpired(11.5)); // elapsed 1.5s <= 2.0s
    EXPECT_TRUE(freshPkt->IsExpired(12.5));  // elapsed 2.5s > 2.0s
}

TEST(PacketEngineTest, DropTailPacketCountLimit) {
    NtnSim::Packet::PacketEngine engine;

    // Configure queue limit to max 3 packets
    engine.ConfigureQueue("sat-queue-1", 3, 10 * 1024 * 1024);
    EXPECT_EQ(engine.GetQueueConfig("sat-queue-1").maxPackets, 3u);

    auto p1 = engine.GeneratePacket(1, 1000, 0.0, "u", "gw");
    auto p2 = engine.GeneratePacket(2, 1000, 0.0, "u", "gw");
    auto p3 = engine.GeneratePacket(3, 1000, 0.0, "u", "gw");
    auto p4 = engine.GeneratePacket(4, 1000, 0.0, "u", "gw");

    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p1, 1.0));
    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p2, 1.0));
    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p3, 1.0));
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 3u);

    // 4th packet must be dropped (DropTail)
    EXPECT_FALSE(engine.EnqueuePacket("sat-queue-1", p4, 1.0));
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 3u);
    EXPECT_EQ(engine.GetDroppedPacketCount("sat-queue-1"), 1u);

    // Dequeue one, then enqueue p4 successfully
    auto popped = engine.DequeuePacket("sat-queue-1", 1.1);
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(popped->GetId(), 1u);
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 2u);

    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p4, 1.1));
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 3u);
}

TEST(PacketEngineTest, DropTailByteCapacityLimit) {
    NtnSim::Packet::PacketEngine engine;

    // Configure queue to max 3000 bytes
    engine.ConfigureQueue("byte-queue", 100, 3000);

    auto p1 = engine.GeneratePacket(10, 1500, 0.0, "u", "gw");
    auto p2 = engine.GeneratePacket(11, 1500, 0.0, "u", "gw");
    auto p3 = engine.GeneratePacket(12, 1000, 0.0, "u", "gw");

    EXPECT_TRUE(engine.EnqueuePacket("byte-queue", p1, 1.0));
    EXPECT_TRUE(engine.EnqueuePacket("byte-queue", p2, 1.0));
    EXPECT_EQ(engine.GetQueueSizeBytes("byte-queue"), 3000u);

    // p3 (1000 bytes) exceeds 3000 bytes capacity -> dropped
    EXPECT_FALSE(engine.EnqueuePacket("byte-queue", p3, 1.0));
    EXPECT_EQ(engine.GetDroppedPacketCount("byte-queue"), 1u);
    EXPECT_EQ(engine.GetQueueSizeBytes("byte-queue"), 3000u);
}

TEST(PacketEngineTest, QueueLatencyCalculation) {
    NtnSim::Packet::PacketEngine engine;

    auto pkt = engine.GeneratePacket(5001, 1500, 1.000, "user-1", "gw-0");

    // Enqueued at t = 1.200s
    EXPECT_TRUE(engine.EnqueuePacket("sat-buffer", pkt, 1.200));
    EXPECT_DOUBLE_EQ(pkt->GetEnqueueTime(), 1.200);

    // Dequeued at t = 1.235s (spent 35 ms in queue)
    auto dequeued = engine.DequeuePacket("sat-buffer", 1.235);
    ASSERT_NE(dequeued, nullptr);
    EXPECT_DOUBLE_EQ(dequeued->GetDepartureTime(), 1.235);
    EXPECT_NEAR(dequeued->GetQueueDelay(), 0.035, 1e-6); // 35 ms delay
    EXPECT_NEAR(engine.GetLastQueueDelay("sat-buffer"), 0.035, 1e-6);

    // Set arrival time and verify end-to-end latency
    dequeued->SetArrivalTime(1.250); // arrived at t = 1.250s, created at t = 1.000s
    EXPECT_NEAR(dequeued->GetEndToEndLatency(), 0.250, 1e-6); // 250 ms total
}

// =========================================================================
// Phase 3 Tests: Traffic Generation Models (CBR & Poisson)
// =========================================================================

TEST(PacketEngineTest, TrafficFlowConfigurationManagement) {
    NtnSim::Packet::PacketEngine engine;

    NtnSim::Packet::TrafficFlowConfig flow1;
    flow1.flowId = "flow-voice-1";
    flow1.sourceId = "user-1";
    flow1.destinationId = "gw-0";
    flow1.pattern = NtnSim::Packet::TrafficPattern::CBR;
    flow1.packetRate = 50.0;
    flow1.packetSizeBytes = 200;

    engine.AddTrafficFlow(flow1);

    auto retrieved = engine.GetTrafficFlow("flow-voice-1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->flowId, "flow-voice-1");
    EXPECT_DOUBLE_EQ(retrieved->packetRate, 50.0);
    EXPECT_EQ(retrieved->packetSizeBytes, 200u);

    EXPECT_EQ(engine.GetAllTrafficFlows().size(), 1u);

    EXPECT_TRUE(engine.RemoveTrafficFlow("flow-voice-1"));
    EXPECT_FALSE(engine.GetTrafficFlow("flow-voice-1").has_value());
    EXPECT_EQ(engine.GetAllTrafficFlows().size(), 0u);
}

TEST(PacketEngineTest, CbrTrafficGenerationWithScheduler) {
    NtnSim::Packet::PacketEngine engine;
    NtnSim::Core::EventScheduler scheduler;

    // CBR: 100 packets/sec -> 0.01s interval between packets.
    // Time window: [0.0s, 0.05s] -> expect 6 generation events at t = 0.00, 0.01, 0.02, 0.03, 0.04, 0.05
    NtnSim::Packet::TrafficFlowConfig config;
    config.flowId = "cbr-flow";
    config.sourceId = "ue-cbr";
    config.destinationId = "gw-0";
    config.pattern = NtnSim::Packet::TrafficPattern::CBR;
    config.packetRate = 100.0; // 10 ms interval
    config.packetSizeBytes = 1500;
    config.startTime = 0.0;
    config.stopTime = 0.0501;

    engine.ScheduleTrafficFlow(config, scheduler);

    // Process all discrete events scheduled
    size_t eventCount = 0;
    while (!scheduler.IsEmpty()) {
        scheduler.ProcessNextEvent();
        eventCount++;
    }

    EXPECT_EQ(eventCount, 6u);
    EXPECT_EQ(engine.GetQueueSize("ue-cbr"), 6u);

    // Dequeue all and verify regular timestamps
    double expectedTime = 0.0;
    for (size_t i = 0; i < 6; ++i) {
        auto pkt = engine.DequeuePacket("ue-cbr", expectedTime);
        ASSERT_NE(pkt, nullptr);
        EXPECT_NEAR(pkt->GetCreationTime(), expectedTime, 1e-5);
        expectedTime += 0.010;
    }
}

TEST(PacketEngineTest, PoissonTrafficGenerationIntervals) {
    NtnSim::Packet::PacketEngine engine;

    // Generate samples for lambda = 50 packets/sec -> expected mean interval = 0.02s
    double rate = 50.0;
    double sumIntervals = 0.0;
    size_t nSamples = 1000;

    for (size_t i = 0; i < nSamples; ++i) {
        double interval = engine.GetNextPoissonInterval(rate);
        EXPECT_GT(interval, 0.0); // Must be strictly positive
        sumIntervals += interval;
    }

    double meanInterval = sumIntervals / static_cast<double>(nSamples);
    double expectedMean = 1.0 / rate; // 0.02s

    // Mean should be within 15% of theoretical expectation for 1000 samples
    EXPECT_NEAR(meanInterval, expectedMean, expectedMean * 0.15);
}

// =========================================================================
// Phase 4 Tests: Transmission & Latency Physics Modeling
// =========================================================================

TEST(PacketEngineTest, PhysicalHopLatencyCalculations) {
    // 1500 bytes over 100 Mbps (100 * 10^6 bps)
    // T_tx = (1500 * 8) / 100,000,000 = 12000 / 100,000,000 = 0.000120 s = 0.12 ms
    double txDelay = NtnSim::Packet::PacketEngine::CalculateTxDelay(1500, 100.0e6);
    EXPECT_NEAR(txDelay, 0.000120, 1e-7);

    // 600 km propagation at speed of light (299,792,458 m/s)
    // T_prop = 600,000 / 299,792,458 ~= 0.00200138 s ~= 2.001 ms
    double propDelay = NtnSim::Packet::PacketEngine::CalculatePropDelay(600000.0);
    EXPECT_NEAR(propDelay, 0.00200138, 1e-6);

    // End-to-end breakdown test
    NtnSim::Packet::PacketEngine engine;
    auto breakdown = engine.CalculateHopLatency(1500, 100.0e6, 600000.0, 0.005, 0.00005);
    EXPECT_DOUBLE_EQ(breakdown.queueDelaySeconds, 0.005);
    EXPECT_NEAR(breakdown.txDelaySeconds, 0.000120, 1e-7);
    EXPECT_NEAR(breakdown.propDelaySeconds, 0.00200138, 1e-6);
    EXPECT_DOUBLE_EQ(breakdown.procDelaySeconds, 0.00005);
    
    double expectedTotal = 0.005 + 0.000120 + 0.00200138 + 0.00005;
    EXPECT_NEAR(breakdown.totalHopLatencySeconds, expectedTotal, 1e-6);
}

TEST(PacketEngineTest, RadioChannelDropOnLowSinr) {
    NtnSim::Packet::PacketEngine engine;

    auto pkt = engine.GeneratePacket(3001, 1500, 0.0, "user-1", "gw-0");

    // Transmit over link with SINR = -5.0 dB against minimum threshold 0.0 dB
    auto res = engine.TransmitHop(pkt, "sat-1", 100.0e6, 600000.0, -5.0, 0.001, 0.0);
    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.dropReason, NtnSim::Metrics::PacketDropReason::RadioChannelLoss);
    EXPECT_EQ(engine.GetTotalPacketsDropped(), 1u);
}

TEST(PacketEngineTest, TtlAndLifetimeExpiryDuringHop) {
    NtnSim::Packet::PacketEngine engine;

    // 1. TTL = 1: 1st hop succeeds (ttl becomes 0), 2nd hop fails with TtlExpired
    auto pktTtl = engine.GeneratePacket(4001, 1000, 0.0, "u-1", "gw-0", 2, 1, 10.0);
    auto res1 = engine.TransmitHop(pktTtl, "sat-1", 100e6, 600000.0, 15.0, 0.001);
    EXPECT_FALSE(res1.success); // Decrement from 1 to 0 causes hop rejection
    EXPECT_EQ(res1.dropReason, NtnSim::Metrics::PacketDropReason::TtlExpired);

    // 2. Max lifetime expired: created at 0.0, maxLifetime = 0.010 (10 ms), transmitted at 0.020 (20 ms)
    auto pktTimeout = engine.GeneratePacket(4002, 1000, 0.0, "u-1", "gw-0", 2, 64, 0.010);
    auto res2 = engine.TransmitHop(pktTimeout, "sat-1", 100e6, 600000.0, 15.0, 0.020);
    EXPECT_FALSE(res2.success);
    EXPECT_EQ(res2.dropReason, NtnSim::Metrics::PacketDropReason::TtlExpired);
}

// =========================================================================
// Phase 5 Tests: Metrics Engine Hook & Event-Driven Delivery
// =========================================================================

TEST(PacketEngineTest, MetricsEngineHookAndKPIs) {
    auto metricsEngine = std::make_shared<NtnSim::Metrics::MetricsEngine>();
    NtnSim::Packet::PacketEngine engine(metricsEngine);

    // Spawn 10 packets -> sent = 10
    std::vector<std::shared_ptr<NtnSim::Packet::Packet>> pkts;
    for (uint64_t i = 1; i <= 10; ++i) {
        pkts.push_back(engine.GeneratePacket(i, 1500, 0.001 * i, "u-1", "gw-0"));
    }
    EXPECT_EQ(engine.GetTotalPacketsSent(), 10u);

    // 7 packets delivered to gateway with 25 ms latency
    for (size_t i = 0; i < 7; ++i) {
        engine.DeliverPacket(pkts[i], pkts[i]->GetCreationTime() + 0.025);
    }
    EXPECT_EQ(engine.GetTotalPacketsDelivered(), 7u);

    // 1 packet dropped via QueueOverflow
    engine.DropPacket(pkts[7], NtnSim::Metrics::PacketDropReason::QueueOverflow, 0.100);

    // 1 packet dropped via RadioChannelLoss
    engine.DropPacket(pkts[8], NtnSim::Metrics::PacketDropReason::RadioChannelLoss, 0.101);

    // 1 packet dropped via TtlExpired
    engine.DropPacket(pkts[9], NtnSim::Metrics::PacketDropReason::TtlExpired, 0.102);

    EXPECT_EQ(engine.GetTotalPacketsDropped(), 3u);

    // Verify MetricsEngine computed Traffic KPIs
    auto kpis = metricsEngine->CalculateTrafficKPIs();
    EXPECT_EQ(kpis.packetsSent, 10u);
    EXPECT_EQ(kpis.packetsReceived, 7u);
    EXPECT_EQ(kpis.packetsDropped, 3u);
    EXPECT_DOUBLE_EQ(kpis.packetDeliveryRatioPercent, 70.0);
    EXPECT_EQ(kpis.dropsQueueOverflow, 1u);
    EXPECT_EQ(kpis.dropsRadioLoss, 1u);
    EXPECT_EQ(kpis.dropsTtlExpired, 1u);
    EXPECT_NEAR(kpis.meanLatencySeconds, 0.025, 1e-6);
}

TEST(PacketEngineTest, EventDrivenHopTransmissionPipeline) {
    auto metricsEngine = std::make_shared<NtnSim::Metrics::MetricsEngine>();
    NtnSim::Packet::PacketEngine engine(metricsEngine);
    NtnSim::Core::EventScheduler scheduler;

    // Create packet destined for gw-0
    auto pkt = engine.GeneratePacket(9999, 1500, 0.0, "user-1", "gw-0", 2, 64, 2.0);

    // Schedule hop 1: user-1 -> sat-1 (intermediate node)
    bool scheduledHop1 = engine.ScheduleHopTransmission(pkt, "sat-1", 100e6, 600000.0, 15.0, 0.0, scheduler);
    EXPECT_TRUE(scheduledHop1);

    // Process hop 1 arrival event
    ASSERT_FALSE(scheduler.IsEmpty());
    scheduler.ProcessNextEvent(); // Triggers RoutePacket at sat-1

    double satArrivalTime = scheduler.GetCurrentTime(); // Arrival at sat-1
    EXPECT_GT(satArrivalTime, 0.0);
    EXPECT_EQ(pkt->GetHopCount(), 1u);
    EXPECT_EQ(pkt->GetHopHistory().back(), "sat-1");

    // Schedule hop 2: sat-1 -> gw-0 (destination)
    bool scheduledHop2 = engine.ScheduleHopTransmission(pkt, "gw-0", 100e6, 600000.0, 15.0, 0.010, scheduler);
    EXPECT_TRUE(scheduledHop2);

    // Process hop 2 arrival event (triggers PacketDelivered)
    ASSERT_FALSE(scheduler.IsEmpty());
    scheduler.ProcessNextEvent();

    EXPECT_EQ(pkt->GetHopCount(), 2u);
    EXPECT_EQ(pkt->GetHopHistory().back(), "gw-0");
    EXPECT_GT(pkt->GetArrivalTime(), 0.010);
    EXPECT_EQ(engine.GetTotalPacketsDelivered(), 1u);

    auto kpis = metricsEngine->CalculateTrafficKPIs();
    EXPECT_EQ(kpis.packetsSent, 1u);
    EXPECT_EQ(kpis.packetsReceived, 1u);
    EXPECT_DOUBLE_EQ(kpis.packetDeliveryRatioPercent, 100.0);
    EXPECT_GT(kpis.meanLatencySeconds, 0.0);
}
