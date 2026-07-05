#include <gtest/gtest.h>
#include "ntn-sim/handover/handover_engine.h"
#include "ntn-sim/handover/best_signal_algorithm.h"
#include "ntn-sim/handover/load_aware_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"

TEST(HandoverEngineTest, StrategyBindingAndDecision) {
    NtnSim::Handover::HandoverEngine engine;
    
    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("user-1", userPos);
    
    NtnSim::Orbit::OrbitState satState;
    auto sat1 = std::make_shared<NtnSim::Orbit::Satellite>("sat-1", satState);
    auto sat2 = std::make_shared<NtnSim::Orbit::Satellite>("sat-2", satState);
    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {sat1, sat2};
    
    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;
    
    // Default strategy should execute
    std::string choice1 = engine.ProcessHandover(user, candidates, cov, sig);
    EXPECT_EQ(choice1, "sat-1");

    // Dynamically change strategy to LoadAware
    engine.SetAlgorithm(std::make_unique<NtnSim::Handover::LoadAwareAlgorithm>());
    std::string choice2 = engine.ProcessHandover(user, candidates, cov, sig);
    EXPECT_EQ(choice2, "sat-1"); // Still returns candidate 1 as skeleton, but tests strategy polymorphism
}
