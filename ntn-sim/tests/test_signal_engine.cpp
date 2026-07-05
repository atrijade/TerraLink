#include <gtest/gtest.h>
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/signal/connection.h"
#include "ntn-sim/coverage/coverage_info.h"

TEST(SignalEngineTest, CalculateSignalAndManageConnections) {
    NtnSim::Signal::SignalEngine engine;
    
    NtnSim::Coverage::CoverageInfo cov;
    auto sig = engine.CalculateSignal(cov, 2.1); // 2.1 GHz (S-band)
    
    EXPECT_DOUBLE_EQ(sig.GetPathLossDb(), 130.0);
    EXPECT_DOUBLE_EQ(sig.GetRsrpDbm(), -85.0);
    
    engine.UpdateConnection("user-1", "sat-1", sig);
    
    auto conn = engine.GetConnection("user-1", "sat-1");
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->IsActive());
    EXPECT_DOUBLE_EQ(conn->GetSignalInfo().GetSinrDb(), 18.0);
}
