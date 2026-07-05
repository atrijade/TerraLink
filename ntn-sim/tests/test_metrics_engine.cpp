#include <gtest/gtest.h>
#include "ntn-sim/metrics/metrics_engine.h"

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
