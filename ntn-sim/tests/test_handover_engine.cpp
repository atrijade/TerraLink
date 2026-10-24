#include <gtest/gtest.h>
#include "ntn-sim/handover/handover_engine.h"
#include "ntn-sim/handover/best_signal_algorithm.h"
#include "ntn-sim/handover/hysteresis_algorithm.h"
#include "ntn-sim/handover/load_aware_algorithm.h"
#include "ntn-sim/handover/future_ai_algorithm.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/utils/math_utils.h"

namespace {

// Helper to construct a test satellite at a given ECEF position and velocity
std::shared_ptr<NtnSim::Orbit::Satellite> CreateTestSatellite(
    const std::string& id,
    const NtnSim::Utils::Position3D& ecefPos,
    const NtnSim::Utils::Position3D& ecefVel = {0.0, 0.0, 0.0}) {

    NtnSim::Orbit::OrbitState state;
    state.SetEcefPosition(ecefPos);
    state.SetEcefVelocity(ecefVel);
    return std::make_shared<NtnSim::Orbit::Satellite>(id, state);
}

} // anonymous namespace

TEST(HandoverEngineTest, StrategyBindingAndDecision) {
    NtnSim::Handover::HandoverEngine engine;
    
    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("user-1", userPos);
    
    // Sat-1 placed at Zenith directly above user at 600 km altitude
    // Earth radius ~6378 km -> Zenith ECEF: (6978137, 0, 0)
    auto sat1 = CreateTestSatellite("sat-1", {6978137.0, 0.0, 0.0});
    auto sat2 = CreateTestSatellite("sat-2", {6978137.0, 100000.0, 0.0});
    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {sat1, sat2};
    
    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;
    
    // Default BestSignal strategy should choose sat-1 (zenith, highest RSRP)
    std::string choice1 = engine.ProcessHandover(user, candidates, cov, sig);
    EXPECT_EQ(choice1, "sat-1");

    // Dynamically change strategy to LoadAware
    engine.SetAlgorithm(std::make_unique<NtnSim::Handover::LoadAwareAlgorithm>());
    std::string choice2 = engine.ProcessHandover(user, candidates, cov, sig);
    EXPECT_EQ(choice2, "sat-1"); // Both unloaded, sat-1 wins on RF
}

TEST(HandoverEngineTest, BestSignalAlgorithmGreedySelection) {
    NtnSim::Handover::BestSignalAlgorithm algo;
    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;

    // User at Prime Meridian Equator (0, 0, 0)
    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-1", userPos);

    // Sat-1: Zenith (distance ~600 km, elevation ~90 deg -> highest RSRP)
    auto sat1 = CreateTestSatellite("sat-zenith", {6978137.0, 0.0, 0.0});

    // Sat-2: Slanted at lower elevation (distance ~1500 km -> lower RSRP)
    auto sat2 = CreateTestSatellite("sat-slanted", {6800000.0, 1400000.0, 0.0});

    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {sat1, sat2};

    // 1. Initial selection connects to zenith satellite
    std::string target = algo.MakeDecision(user, candidates, cov, sig);
    EXPECT_EQ(target, "sat-zenith");

    // 2. User connected to slanted satellite -> greedy switch to zenith satellite
    user.SetServingSatelliteId("sat-slanted");
    target = algo.MakeDecision(user, candidates, cov, sig);
    EXPECT_EQ(target, "sat-zenith");

    // 3. User already connected to zenith satellite -> retain zenith
    user.SetServingSatelliteId("sat-zenith");
    target = algo.MakeDecision(user, candidates, cov, sig);
    EXPECT_EQ(target, "sat-zenith");
}

TEST(HandoverEngineTest, HysteresisPreventsPingPongOnSmallMargin) {
    NtnSim::Handover::HysteresisAlgorithm algo(/*marginDb=*/3.0, /*tttSeconds=*/2.0);
    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-pingpong", userPos);

    // Sat-1: Serving satellite at distance ~800 km
    auto sat1 = CreateTestSatellite("sat-serving", {7178137.0, 0.0, 0.0});
    user.SetServingSatelliteId("sat-serving");

    // Sat-2: Candidate satellite positioned slightly closer (e.g. distance ~750 km)
    // yielding an RSRP advantage of only ~0.6 dB (below the 3.0 dB margin)
    auto sat2 = CreateTestSatellite("sat-candidate", {7128137.0, 0.0, 0.0});

    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {sat1, sat2};

    // Even though sat-2 is slightly stronger, margin condition (delta > 3 dB) is NOT met!
    // Handover must be suppressed to avoid ping-pong.
    std::string target = algo.MakeDecision(user, candidates, cov, sig, 1.0);
    EXPECT_EQ(target, "sat-serving");
    EXPECT_FALSE(algo.IsTriggerActive("ue-pingpong"));
}

TEST(HandoverEngineTest, HysteresisTimeToTriggerWindow) {
    // 3GPP A3 Handover: Margin = 2.0 dB, TTT = 2.0 seconds
    NtnSim::Handover::HysteresisAlgorithm algo(/*marginDb=*/2.0, /*tttSeconds=*/2.0);
    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-ttt", userPos);

    // Sat-serving at 1200 km
    auto satServing = CreateTestSatellite("sat-serving", {7578137.0, 0.0, 0.0});
    user.SetServingSatelliteId("sat-serving");

    // Sat-better at 600 km (distance halved -> ~6 dB RSRP advantage, exceeding 2 dB margin)
    auto satBetter = CreateTestSatellite("sat-better", {6978137.0, 0.0, 0.0});

    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {satServing, satBetter};

    // t = 10.0s: Condition enters A3 window for the first time -> TTT timer starts
    std::string decision1 = algo.MakeDecision(user, candidates, cov, sig, 10.0);
    EXPECT_EQ(decision1, "sat-serving"); // Still serving during TTT
    EXPECT_TRUE(algo.IsTriggerActive("ue-ttt"));

    // t = 11.5s: Elapsed = 1.5s < 2.0s -> Handover still not triggered
    std::string decision2 = algo.MakeDecision(user, candidates, cov, sig, 11.5);
    EXPECT_EQ(decision2, "sat-serving");
    EXPECT_TRUE(algo.IsTriggerActive("ue-ttt"));

    // t = 12.1s: Elapsed = 2.1s >= 2.0s -> TTT fulfilled! Handover authorized to sat-better
    std::string decision3 = algo.MakeDecision(user, candidates, cov, sig, 12.1);
    EXPECT_EQ(decision3, "sat-better");
    EXPECT_FALSE(algo.IsTriggerActive("ue-ttt")); // Trigger reset
}

TEST(HandoverEngineTest, HysteresisRadioLinkFailureImmediateRecovery) {
    NtnSim::Handover::HysteresisAlgorithm algo(/*marginDb=*/3.0, /*tttSeconds=*/2.0);
    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-rlf", userPos);

    // User was connected to sat-lost, but sat-lost is no longer in visible candidates
    user.SetServingSatelliteId("sat-lost");

    auto satRising = CreateTestSatellite("sat-rising", {6978137.0, 0.0, 0.0});
    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {satRising};

    // Immediate recovery: No TTT wait required when serving link is completely lost
    std::string decision = algo.MakeDecision(user, candidates, cov, sig, 5.0);
    EXPECT_EQ(decision, "sat-rising");
}

TEST(HandoverEngineTest, LoadAwareCongestionAvoidance) {
    auto algo = std::make_unique<NtnSim::Handover::LoadAwareAlgorithm>(
        /*maxCapacityPerSatellite=*/100, /*rfWeight=*/0.5, /*loadWeight=*/0.5);

    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-load", userPos);

    // Sat-1: Slightly better RF (at 600 km), BUT 100% congested (100 connected UEs)
    auto satCongested = CreateTestSatellite("sat-congested", {6978137.0, 0.0, 0.0});
    algo->SetSatelliteLoad("sat-congested", 100);

    // Sat-2: Good RF (at 700 km), lightly loaded (only 10 connected UEs)
    auto satLight = CreateTestSatellite("sat-light", {7078137.0, 0.0, 0.0});
    algo->SetSatelliteLoad("sat-light", 10);

    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {satCongested, satLight};

    // Load-aware algorithm balances load and diverts user to sat-light
    std::string choice = algo->MakeDecision(user, candidates, cov, sig);
    EXPECT_EQ(choice, "sat-light");
}

TEST(HandoverEngineTest, FutureAIPrioritizesRisingOverSetting) {
    auto algo = std::make_unique<NtnSim::Handover::FutureAIAlgorithm>(
        /*carrierFrequencyGhz=*/2.0, /*trajectoryWeight=*/0.6);

    NtnSim::Coverage::CoverageEngine cov;
    NtnSim::Signal::SignalEngine sig;

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-ai", userPos);

    // Both satellites placed at similar symmetric distance (elevation ~45 deg)
    // Sat-Setting: Velocity vector moving AWAY from user (-LosVelocity)
    auto satSetting = CreateTestSatellite("sat-setting",
                                          {6700000.0, 1000000.0, 0.0},
                                          {0.0, 7.5, 0.0}); // Moving East (away)

    // Sat-Rising: Velocity vector moving TOWARDS user (+LosVelocity)
    auto satRising = CreateTestSatellite("sat-rising",
                                         {6700000.0, -1000000.0, 0.0},
                                         {0.0, 7.5, 0.0}); // Moving East (toward meridian)

    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> candidates = {satSetting, satRising};

    // FutureAI prioritizes the rising satellite with longer future service window
    std::string decision = algo->MakeDecision(user, candidates, cov, sig);
    EXPECT_EQ(decision, "sat-rising");
}

TEST(HandoverEngineTest, ExecuteHandoverAndTelemetryIntegration) {
    auto metricsEngine = std::make_shared<NtnSim::Metrics::MetricsEngine>();
    NtnSim::Handover::HandoverEngine engine;
    engine.SetMetricsEngine(metricsEngine);

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("ue-exec", userPos);

    // 1. Initial connection to sat-1 at t = 0.0s (not a handover from previous satellite)
    user.SetServingSatelliteId("sat-1");

    // 2. Handover from sat-1 -> sat-2 at t = 10.0s
    bool h1 = engine.ExecuteHandover(user, "sat-2", 10.0, 0.035);
    EXPECT_TRUE(h1);
    EXPECT_EQ(user.GetServingSatelliteId(), "sat-2");
    EXPECT_EQ(engine.GetTotalHandoversExecuted(), 1u);

    // 3. Fast ping-pong switch back to sat-1 at t = 12.0s (elapsed = 2.0s < 5.0s window)
    bool h2 = engine.ExecuteHandover(user, "sat-1", 12.0, 0.035);
    EXPECT_TRUE(h2);
    EXPECT_EQ(user.GetServingSatelliteId(), "sat-1");
    EXPECT_EQ(engine.GetTotalHandoversExecuted(), 2u);

    // 4. Duplicate target -> No handover executed
    bool h3 = engine.ExecuteHandover(user, "sat-1", 15.0);
    EXPECT_FALSE(h3);
    EXPECT_EQ(engine.GetTotalHandoversExecuted(), 2u);

    // 5. Complete loss of coverage (target = "") -> Failed handover recorded
    bool h4 = engine.ExecuteHandover(user, "", 20.0);
    EXPECT_FALSE(h4);
    EXPECT_TRUE(user.GetServingSatelliteId().empty());

    // Verify MetricsEngine telemetry analytics
    auto kpis = metricsEngine->CalculateHandoverKPIs(/*pingPongThresholdSeconds=*/5.0);
    EXPECT_EQ(kpis.totalAttempts, 3u); // 2 successful handovers + 1 drop to empty
    EXPECT_EQ(kpis.totalSuccesses, 2u);
    EXPECT_EQ(kpis.totalFailures, 1u);
    EXPECT_EQ(kpis.pingPongCount, 1u); // sat-1 -> sat-2 -> sat-1 in 2 seconds!
    EXPECT_NEAR(kpis.pingPongRatePercent, 33.3333, 1e-3); // 1 pingpong out of 3 total attempts
    EXPECT_NEAR(kpis.averageInterruptionTimeSeconds, 0.035, 1e-4);
}
