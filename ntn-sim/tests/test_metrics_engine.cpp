#include <gtest/gtest.h>
#include "ntn-sim/metrics/metrics_engine.h"
#include <cmath>
#include <thread>
#include <vector>

TEST(MetricsEngineTest, LogAndAggregateKPIs) {
    NtnSim::Metrics::MetricsEngine engine;
    
    engine.RecordMetric("latency", 0.02, 1.0);
    engine.RecordMetric("latency", 0.04, 2.0);
    engine.RecordMetric("handover_count", 1.0, 3.0);
    
    EXPECT_DOUBLE_EQ(engine.GetAverageMetric("latency"), 0.03);
    EXPECT_DOUBLE_EQ(engine.GetAverageMetric("handover_count"), 1.0);
    EXPECT_DOUBLE_EQ(engine.GetAverageMetric("non-existent"), 0.0);
    
    engine.Reset();
    EXPECT_DOUBLE_EQ(engine.GetAverageMetric("latency"), 0.0);
}

TEST(MetricsEngineTest, SummaryStatisticsAndPercentiles) {
    NtnSim::Metrics::MetricsEngine engine;

    // 5 samples: 10, 20, 30, 40, 50
    engine.RecordMetric("rtt", 10.0, 1.0);
    engine.RecordMetric("rtt", 20.0, 2.0);
    engine.RecordMetric("rtt", 30.0, 3.0);
    engine.RecordMetric("rtt", 40.0, 4.0);
    engine.RecordMetric("rtt", 50.0, 5.0);

    EXPECT_EQ(engine.GetCount("rtt"), 5u);
    EXPECT_DOUBLE_EQ(engine.GetMin("rtt"), 10.0);
    EXPECT_DOUBLE_EQ(engine.GetMax("rtt"), 50.0);
    EXPECT_DOUBLE_EQ(engine.GetAverageMetric("rtt"), 30.0);

    // Sample variance of [10, 20, 30, 40, 50] is 250.0, StDev is sqrt(250) ≈ 15.811388
    EXPECT_DOUBLE_EQ(engine.GetVariance("rtt"), 250.0);
    EXPECT_NEAR(engine.GetStandardDeviation("rtt"), 15.811388, 1e-5);

    // Percentiles
    EXPECT_DOUBLE_EQ(engine.GetPercentile("rtt", 0.0), 10.0);
    EXPECT_DOUBLE_EQ(engine.GetPercentile("rtt", 50.0), 30.0); // Median
    EXPECT_DOUBLE_EQ(engine.GetPercentile("rtt", 100.0), 50.0);
    // 95th percentile: k = 0.95 * 4 = 3.8 -> 40 + 0.8 * 10 = 48.0
    EXPECT_DOUBLE_EQ(engine.GetPercentile("rtt", 95.0), 48.0);
}

TEST(MetricsEngineTest, TimeWindowFiltering) {
    NtnSim::Metrics::MetricsEngine engine;

    engine.RecordMetric("elevation", 15.0, 1.0);
    engine.RecordMetric("elevation", 30.0, 5.0);
    engine.RecordMetric("elevation", 60.0, 10.0);
    engine.RecordMetric("elevation", 20.0, 20.0);

    // Query window [4.0, 12.0] -> includes t=5.0 (30.0) and t=10.0 (60.0)
    auto samples = engine.GetSamplesInTimeWindow("elevation", 4.0, 12.0);
    EXPECT_EQ(samples.size(), 2u);
    EXPECT_DOUBLE_EQ(engine.GetAverageMetricInWindow("elevation", 4.0, 12.0), 45.0);

    // Window with no samples
    EXPECT_DOUBLE_EQ(engine.GetAverageMetricInWindow("elevation", 50.0, 60.0), 0.0);
}

TEST(MetricsEngineTest, HandoverKPIsAndPingPongDetection) {
    NtnSim::Metrics::MetricsEngine engine;

    // User 1 executes:
    // 1) sat1 -> sat2 at t=10.0 (success, interruption = 0.03s)
    engine.RecordHandover("user-1", "sat1", "sat2", 10.0, true, 0.03);

    // 2) sat2 -> sat1 at t=12.0 (success, interruption = 0.02s) -> dt = 2.0s <= 5.0s -> PING-PONG!
    engine.RecordHandover("user-1", "sat2", "sat1", 12.0, true, 0.02);

    // 3) sat1 -> sat2 at t=35.0 (failure, radio link lost) -> dt = 23.0s > 5.0s (not ping-pong)
    engine.RecordHandover("user-1", "sat1", "sat2", 35.0, false, 0.0);

    auto kpis = engine.CalculateHandoverKPIs(5.0);

    EXPECT_EQ(kpis.totalAttempts, 3u);
    EXPECT_EQ(kpis.totalSuccesses, 2u);
    EXPECT_EQ(kpis.totalFailures, 1u);
    EXPECT_NEAR(kpis.successRatePercent, 66.6667, 1e-3);

    EXPECT_EQ(kpis.pingPongCount, 1u);
    EXPECT_NEAR(kpis.pingPongRatePercent, 33.3333, 1e-3);

    EXPECT_DOUBLE_EQ(kpis.averageInterruptionTimeSeconds, 0.025);
    EXPECT_DOUBLE_EQ(kpis.maxInterruptionTimeSeconds, 0.03);
}

TEST(MetricsEngineTest, RadioKPIsOutageAndAvailability) {
    NtnSim::Metrics::MetricsEngine engine;

    // 4 samples:
    // 1: Normal good link
    engine.RecordRadioSample("u1", "sat1", -80.0, 15.0, 1000.0, true, 1.0);
    // 2: Cell-edge acceptable
    engine.RecordRadioSample("u1", "sat1", -90.0, 8.0, -1800.0, true, 2.0);
    // 3: Outage condition (SINR -6.0 dB < threshold -3.0 dB)
    engine.RecordRadioSample("u1", "sat1", -110.0, -6.0, 500.0, true, 3.0);
    // 4: Complete out-of-coverage
    engine.RecordRadioSample("u1", "sat1", -140.0, -30.0, 0.0, false, 4.0);

    auto kpis = engine.CalculateRadioKPIs(-3.0);

    EXPECT_NEAR(kpis.meanRsrpDbm, -105.0, 0.01);
    EXPECT_LT(kpis.p10RsrpDbm, -110.0); // 10th percentile near edge
    EXPECT_DOUBLE_EQ(kpis.maxDopplerShiftHz, 1800.0);
    EXPECT_DOUBLE_EQ(kpis.serviceAvailabilityPercent, 75.0); // 3 out of 4
    EXPECT_DOUBLE_EQ(kpis.outageProbabilityPercent, 50.0);   // 2 out of 4
}

TEST(MetricsEngineTest, TrafficKPIsAndPDR) {
    NtnSim::Metrics::MetricsEngine engine;

    // Send 5 packets (1000 bytes each)
    for (uint64_t i = 1; i <= 5; ++i) {
        engine.RecordPacketSent(i, 1000, 1.0 * static_cast<double>(i));
    }

    // Receive packets 1, 2, 3, 4 with slight delay
    engine.RecordPacketReceived(1, 0.010, 1.010);
    engine.RecordPacketReceived(2, 0.014, 2.014); // delay diff = 0.004
    engine.RecordPacketReceived(3, 0.011, 3.011); // delay diff = 0.003
    engine.RecordPacketReceived(4, 0.015, 4.015); // delay diff = 0.004

    // Drop packet 5 due to QueueOverflow
    engine.RecordPacketDropped(5, NtnSim::Metrics::PacketDropReason::QueueOverflow, 5.0);

    auto kpis = engine.CalculateTrafficKPIs();

    EXPECT_EQ(kpis.packetsSent, 5u);
    EXPECT_EQ(kpis.packetsReceived, 4u);
    EXPECT_EQ(kpis.packetsDropped, 1u);
    EXPECT_DOUBLE_EQ(kpis.packetDeliveryRatioPercent, 80.0); // 4 / 5 = 80%
    EXPECT_EQ(kpis.dropsQueueOverflow, 1u);
    EXPECT_EQ(kpis.dropsRadioLoss, 0u);

    // Latency stats: [0.010, 0.011, 0.014, 0.015] -> mean = 0.0125
    EXPECT_DOUBLE_EQ(kpis.meanLatencySeconds, 0.0125);
    EXPECT_DOUBLE_EQ(kpis.p50LatencySeconds, 0.0125);
    EXPECT_GT(kpis.p95LatencySeconds, 0.014);

    // Jitter: (0.004 + 0.003 + 0.004) / 3 = 0.0036667
    EXPECT_NEAR(kpis.jitterSeconds, 0.0036667, 1e-6);

    // Throughput > 0
    EXPECT_GT(kpis.throughputMbps, 0.0);
}

TEST(MetricsEngineTest, HighThroughputMultithreadedBuffering) {
    NtnSim::Metrics::MetricsEngine engine;

    constexpr int NUM_THREADS = 8;
    constexpr int SAMPLES_PER_THREAD = 1000;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&engine, t]() {
            for (int i = 0; i < SAMPLES_PER_THREAD; ++i) {
                double val = static_cast<double>(t * 100 + i);
                double time = 0.001 * static_cast<double>(i);
                engine.RecordMetric("throughput_sample", val, time);
                
                uint64_t pktId = static_cast<uint64_t>(t * 100000 + i);
                engine.RecordPacketSent(pktId, 1024, time);
                engine.RecordPacketReceived(pktId, 0.010 + 0.0001 * t, time + 0.010);

                if (i % 100 == 0) {
                    // Concurrent read lock query while writer threads are active
                    EXPECT_GE(engine.GetCount("throughput_sample"), 1u);
                    EXPECT_GE(engine.GetAverageMetric("throughput_sample"), 0.0);
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Verify all 8,000 samples were recorded without loss or data race
    EXPECT_EQ(engine.GetCount("throughput_sample"), static_cast<size_t>(NUM_THREADS * SAMPLES_PER_THREAD));

    auto traffic = engine.CalculateTrafficKPIs();
    EXPECT_EQ(traffic.packetsSent, static_cast<size_t>(NUM_THREADS * SAMPLES_PER_THREAD));
    EXPECT_EQ(traffic.packetsReceived, static_cast<size_t>(NUM_THREADS * SAMPLES_PER_THREAD));
    EXPECT_DOUBLE_EQ(traffic.packetDeliveryRatioPercent, 100.0);
}

