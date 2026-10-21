#include <gtest/gtest.h>
#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/signal/connection.h"
#include "ntn-sim/coverage/coverage_info.h"
#include <cmath>

// ---------------------------------------------------------------------------
// Helper: build a CoverageInfo for a satellite directly overhead at 600 km
// ---------------------------------------------------------------------------
static NtnSim::Coverage::CoverageInfo MakeZenithCoverage(double distanceKm = 600.0,
                                                          double elevDeg    = 90.0,
                                                          double losVelKms  = 0.0) {
    NtnSim::Coverage::CoverageInfo cov;
    cov.SetVisible(true);
    cov.SetDistanceMeters(distanceKm * 1000.0);
    cov.SetElevationAngleDeg(elevDeg);
    cov.SetLosVelocityKms(losVelKms);
    return cov;
}

// ---------------------------------------------------------------------------
// Test 1: FSPL benchmark — 600 km, 2.1 GHz S-Band
//   FSPL = 20*log10(600000) + 20*log10(2.1) + 32.45 ≈ 154.43 dB
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, FsplBenchmark_600km_SBand) {
    NtnSim::Signal::SignalEngine engine;
    auto cov = MakeZenithCoverage(600.0, 90.0, 0.0);

    auto sig = engine.CalculateSignal(cov, 2.1);

    // Expected: ~154.43 dB path loss (FSPL + atmos + misc).
    // At zenith: atmos = 0.5/sin(90) = 0.5 dB, misc = 2.0 dB -> total ~156.93 dB
    EXPECT_NEAR(sig.GetPathLossDb(), 156.93, 0.5);
}

// ---------------------------------------------------------------------------
// Test 2: RSRP at 600 km zenith, 2.1 GHz
//   EIRP = 43 + 15 = 58 dBm, RSRP = 58 + 0 - 156.93 ≈ -98.93 dBm
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, RsrpBenchmark_600km_Zenith) {
    NtnSim::Signal::SignalEngine engine;
    auto cov = MakeZenithCoverage(600.0, 90.0, 0.0);

    auto sig = engine.CalculateSignal(cov, 2.1);

    // RSRP ≈ -98.93 dBm (valid NTN downlink budget)
    EXPECT_NEAR(sig.GetRsrpDbm(), -98.93, 0.5);
    // RSRP must be above absolute floor of -140 dBm
    EXPECT_GT(sig.GetRsrpDbm(), -140.0);
}

// ---------------------------------------------------------------------------
// Test 3: Horizon vs. Zenith — low elevation must have worse signal
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, HorizonVsZenith_LowerSignalAtLowElevation) {
    NtnSim::Signal::SignalEngine engine;

    // Satellite at 1800 km slant range near horizon (15 deg elevation)
    auto covHorizon = MakeZenithCoverage(1800.0, 15.0, 0.0);
    // Satellite directly overhead at 600 km
    auto covZenith  = MakeZenithCoverage(600.0,  90.0, 0.0);

    auto sigHorizon = engine.CalculateSignal(covHorizon, 2.1);
    auto sigZenith  = engine.CalculateSignal(covZenith,  2.1);

    // Near-horizon path loss must be significantly higher (worse) than zenith
    EXPECT_GT(sigHorizon.GetPathLossDb(), sigZenith.GetPathLossDb());
    // RSRP must be lower (worse) near horizon
    EXPECT_LT(sigHorizon.GetRsrpDbm(), sigZenith.GetRsrpDbm());
    // Latency must be longer near horizon (greater slant range)
    EXPECT_GT(sigHorizon.GetLatencySeconds(), sigZenith.GetLatencySeconds());
}

// ---------------------------------------------------------------------------
// Test 4: Propagation delay is d / c
//   At 600 km: tau = 600000 m / 3e8 m/s = 2.0 ms
//   At 1800 km: tau = 1800000 / 3e8 = 6.0 ms
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, PropagationDelay_SpeedOfLight) {
    NtnSim::Signal::SignalEngine engine;

    auto cov600  = MakeZenithCoverage(600.0,  90.0);
    auto cov1800 = MakeZenithCoverage(1800.0, 90.0);

    auto sig600  = engine.CalculateSignal(cov600,  2.1);
    auto sig1800 = engine.CalculateSignal(cov1800, 2.1);

    EXPECT_NEAR(sig600.GetLatencySeconds(),  0.002, 1e-5);  // 2.0 ms
    EXPECT_NEAR(sig1800.GetLatencySeconds(), 0.006, 1e-5);  // 6.0 ms
}

// ---------------------------------------------------------------------------
// Test 5: Doppler shift — approaching satellite at 7.558 km/s at 2.1 GHz
//   Delta_f = 2.1e9 * 7558.0 / 3e8 ≈ 52906 Hz ≈ ~52.9 kHz (upshift)
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, DopplerShift_ApproachingSatellite) {
    NtnSim::Signal::SignalEngine engine;

    // Satellite approaching at 7.558 km/s (worst-case head-on)
    auto cov = MakeZenithCoverage(600.0, 90.0, 7.558);
    auto sig = engine.CalculateSignal(cov, 2.1);

    // Positive Doppler = frequency upshift (approaching)
    EXPECT_NEAR(sig.GetDopplerShiftHz(), 52906.0, 200.0);
    EXPECT_GT(sig.GetDopplerShiftHz(), 0.0);

    // Receding satellite at same speed: equal magnitude but negative
    auto covReceding = MakeZenithCoverage(600.0, 90.0, -7.558);
    auto sigReceding = engine.CalculateSignal(covReceding, 2.1);
    EXPECT_NEAR(sigReceding.GetDopplerShiftHz(), -52906.0, 200.0);
    EXPECT_LT(sigReceding.GetDopplerShiftHz(), 0.0);
}

// ---------------------------------------------------------------------------
// Test 6: Out-of-coverage — below minimum elevation angle
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, OutOfCoverage_BelowMinElevation) {
    NtnSim::Signal::SignalEngine engine;

    NtnSim::Coverage::CoverageInfo cov;
    cov.SetVisible(false);
    cov.SetDistanceMeters(2000000.0);
    cov.SetElevationAngleDeg(5.0);  // Below 10 deg minimum

    auto sig = engine.CalculateSignal(cov, 2.1);

    EXPECT_DOUBLE_EQ(sig.GetRsrpDbm(), -140.0);
    EXPECT_DOUBLE_EQ(sig.GetSinrDb(),   -30.0);
    EXPECT_DOUBLE_EQ(sig.GetPathLossDb(), 300.0);
}

// ---------------------------------------------------------------------------
// Test 7: Custom ChannelConfig (VSAT dish user, Ka-Band)
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, CustomChannelConfig_KaBandVSAT) {
    NtnSim::Signal::ChannelConfig cfg;
    cfg.txPowerDbm     = 50.0;   // Higher power
    cfg.txAntennaGainDbi = 30.0; // High-gain satellite antenna
    cfg.rxAntennaGainDbi = 25.0; // VSAT dish gain
    cfg.noiseFigureDb    = 5.0;
    cfg.bandwidthMhz     = 500.0;
    cfg.miscLossDb       = 3.0;

    NtnSim::Signal::SignalEngine engine(cfg);
    auto cov = MakeZenithCoverage(600.0, 90.0, 0.0);

    // Ka-Band at 20 GHz
    auto sig = engine.CalculateSignal(cov, 20.0);

    // Path loss higher at Ka-Band (higher frequency), RSRP still reasonable due to high gains
    EXPECT_GT(sig.GetPathLossDb(), 160.0);
    EXPECT_GT(sig.GetRsrpDbm(), -80.0);  // High gains compensate

    // Verify config round-trip
    auto retrievedCfg = engine.GetChannelConfig();
    EXPECT_DOUBLE_EQ(retrievedCfg.txPowerDbm, 50.0);
    EXPECT_DOUBLE_EQ(retrievedCfg.rxAntennaGainDbi, 25.0);
}

// ---------------------------------------------------------------------------
// Test 8: Connection management — active/inactive state
// ---------------------------------------------------------------------------
TEST(SignalEngineTest, ConnectionManagement_ActiveAndInactive) {
    NtnSim::Signal::SignalEngine engine;

    auto covGood = MakeZenithCoverage(600.0, 90.0, 0.0);
    auto sigGood = engine.CalculateSignal(covGood, 2.1);
    engine.UpdateConnection("user-1", "sat-p0-s0", sigGood);

    auto conn = engine.GetConnection("user-1", "sat-p0-s0");
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->IsActive());
    EXPECT_NEAR(conn->GetSignalInfo().GetLatencySeconds(), 0.002, 1e-5);

    // Out-of-coverage update marks connection inactive
    NtnSim::Coverage::CoverageInfo covBad;
    covBad.SetVisible(false);
    auto sigBad = engine.CalculateSignal(covBad, 2.1);
    engine.UpdateConnection("user-1", "sat-p0-s0", sigBad);

    auto connAfter = engine.GetConnection("user-1", "sat-p0-s0");
    ASSERT_NE(connAfter, nullptr);
    EXPECT_FALSE(connAfter->IsActive());

    // Non-existent connection
    EXPECT_EQ(engine.GetConnection("user-x", "sat-x"), nullptr);
}

