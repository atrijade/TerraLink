#include <gtest/gtest.h>
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/utils/math_utils.h"
#include <cmath>
#include <algorithm>
#include <string>

// ---------------------------------------------------------------------------
// Step 1a: GeodeticToEcef — WGS-84 known ground-truth verification
// ---------------------------------------------------------------------------
TEST(CoverageEngineTest, GeodeticToEcef_EquatorialPrimeOrigin) {
    // (0 lat, 0 lon, 0 alt) should map to (WGS84_A, 0, 0)
    NtnSim::Utils::GeodeticPosition pos{0.0, 0.0, 0.0};
    auto ecef = NtnSim::Utils::MathUtils::GeodeticToEcef(pos);
    EXPECT_NEAR(ecef.x, 6378137.0, 1.0);  // WGS-84 semi-major axis (m)
    EXPECT_NEAR(ecef.y, 0.0, 1.0);
    EXPECT_NEAR(ecef.z, 0.0, 1.0);
}

TEST(CoverageEngineTest, GeodeticToEcef_NorthPole) {
    // (90 lat, 0 lon, 0 alt) -> (0, 0, WGS84_B)  (semi-minor axis)
    NtnSim::Utils::GeodeticPosition pos{90.0, 0.0, 0.0};
    auto ecef = NtnSim::Utils::MathUtils::GeodeticToEcef(pos);
    EXPECT_NEAR(ecef.x, 0.0, 1.0);
    EXPECT_NEAR(ecef.y, 0.0, 1.0);
    EXPECT_NEAR(ecef.z, 6356752.3, 1.0);  // WGS-84 semi-minor axis b
}

TEST(CoverageEngineTest, GeodeticToEcef_KnownCity_London) {
    // London: lat=51.5074, lon=-0.1278, alt=0 m
    // Precise WGS-84 ellipsoid: X=3977994.3 m, Y=-8871.4 m, Z=4968874.9 m
    NtnSim::Utils::GeodeticPosition pos{51.5074, -0.1278, 0.0};
    auto ecef = NtnSim::Utils::MathUtils::GeodeticToEcef(pos);
    EXPECT_NEAR(ecef.x, 3977994.3, 10.0);
    EXPECT_NEAR(ecef.y,   -8871.4, 10.0);
    EXPECT_NEAR(ecef.z, 4968874.9, 10.0);
}

// ---------------------------------------------------------------------------
// Step 1b: CalculateDistance — Euclidean slant range
// ---------------------------------------------------------------------------
TEST(CoverageEngineTest, CalculateDistance_KnownPoints) {
    NtnSim::Utils::Position3D p1{0.0, 0.0, 0.0};
    NtnSim::Utils::Position3D p2{3.0, 4.0, 0.0};  // 3-4-5 triangle
    EXPECT_DOUBLE_EQ(NtnSim::Utils::MathUtils::CalculateDistance(p1, p2), 5.0);
}

TEST(CoverageEngineTest, CalculateDistance_600km) {
    // User at Earth surface, satellite 600 km directly above (on Z axis)
    NtnSim::Utils::Position3D user{6378137.0, 0.0, 0.0};
    NtnSim::Utils::Position3D sat{ 6978137.0, 0.0, 0.0}; // +600 km
    double d = NtnSim::Utils::MathUtils::CalculateDistance(user, sat);
    EXPECT_NEAR(d, 600000.0, 1.0);  // 600 km = 600,000 m
}

// ---------------------------------------------------------------------------
// Step 1c: CalculateElevationAngle — local zenith projection
// ---------------------------------------------------------------------------
TEST(CoverageEngineTest, ElevationAngle_SatelliteDirectlyOverhead) {
    // Satellite directly above equatorial prime meridian point
    NtnSim::Utils::Position3D ground{6378137.0, 0.0, 0.0};
    NtnSim::Utils::Position3D sat{  6978137.0, 0.0, 0.0};  // Same radial direction
    double elev = NtnSim::Utils::MathUtils::CalculateElevationAngle(ground, sat);
    EXPECT_NEAR(elev, 90.0, 0.01);
}

TEST(CoverageEngineTest, ElevationAngle_SatelliteAtHorizon) {
    // Satellite perpendicular to local zenith => elevation ~0 deg
    NtnSim::Utils::Position3D ground{6378137.0, 0.0, 0.0};  // On X-axis
    NtnSim::Utils::Position3D sat{  6378137.0, 6978137.0, 0.0};  // Far out on Y-axis
    double elev = NtnSim::Utils::MathUtils::CalculateElevationAngle(ground, sat);
    EXPECT_NEAR(elev, 0.0, 1.0);  // Near-horizon
}

TEST(CoverageEngineTest, ElevationAngle_SatelliteBelowHorizon) {
    // Satellite behind Earth from the user perspective (negative elevation)
    NtnSim::Utils::Position3D ground{6378137.0, 0.0, 0.0};
    NtnSim::Utils::Position3D sat{  -6978137.0, 0.0, 0.0};  // Opposite side
    double elev = NtnSim::Utils::MathUtils::CalculateElevationAngle(ground, sat);
    EXPECT_LT(elev, 0.0);
}

// ---------------------------------------------------------------------------
// Step 2: CalculateCoverage — full per-pair geometry pipeline
// ---------------------------------------------------------------------------
TEST(CoverageEngineTest, CalculateCoverage_ZenithSatellite) {
    NtnSim::Coverage::CoverageEngine engine;

    // User at equatorial prime meridian (0, 0, 0 geodetic)
    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("user-0", userPos);

    // Satellite propagated to 600 km directly above (0, 0) in ECEF
    NtnSim::Orbit::OrbitState state;
    auto userEcef = NtnSim::Utils::MathUtils::GeodeticToEcef(userPos);
    // Place satellite along same radial direction, 600 km higher
    double scale = (6378137.0 + 600000.0) / 6378137.0;
    NtnSim::Utils::Position3D satPos{userEcef.x * scale, userEcef.y * scale, userEcef.z * scale};
    state.SetEcefPosition(satPos);

    NtnSim::Orbit::Satellite sat("sat-zenith", state);
    auto info = engine.CalculateCoverage(sat, user);

    EXPECT_TRUE(info.IsVisible());
    EXPECT_NEAR(info.GetDistanceMeters(), 600000.0, 100.0);  // ±100 m
    EXPECT_NEAR(info.GetElevationAngleDeg(), 90.0, 0.5);
}

TEST(CoverageEngineTest, CalculateCoverage_BelowHorizon_NotVisible) {
    NtnSim::Coverage::CoverageEngine engine;

    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("user-0", userPos);

    // Satellite placed at 5 deg elevation — below 10 deg minimum
    NtnSim::Orbit::OrbitState state;
    auto userEcef = NtnSim::Utils::MathUtils::GeodeticToEcef(userPos);
    // Shift satellite mostly sideways — very low elevation angle
    NtnSim::Utils::Position3D satPos{userEcef.x + 100000.0, 7000000.0, 0.0};
    state.SetEcefPosition(satPos);

    NtnSim::Orbit::Satellite sat("sat-low", state);
    auto info = engine.CalculateCoverage(sat, user);

    EXPECT_FALSE(info.IsVisible());
}

TEST(CoverageEngineTest, GetVisibleSatellites_Walker72) {
    NtnSim::Orbit::OrbitEngine orbitEngine;
    orbitEngine.GenerateWalkerConstellation(6, 12, 600.0, 87.5, 1);
    orbitEngine.UpdatePositions(0.0);

    NtnSim::Coverage::CoverageEngine coverageEngine;

    // At (0, 0, 0) equatorial prime meridian, sat-p0-s0 is directly overhead at t=0
    NtnSim::User::User userEquator("user-eq", NtnSim::Utils::GeodeticPosition{0.0, 0.0, 0.0});
    auto visible = coverageEngine.GetVisibleSatellites(userEquator, orbitEngine.GetAllSatellites());
    EXPECT_GE(visible.size(), 1u);
    EXPECT_NE(std::find(visible.begin(), visible.end(), "sat-p0-s0"), visible.end());
}

TEST(CoverageEngineTest, GetBestSatellite_SelectsHighestElevation) {
    NtnSim::Coverage::CoverageEngine coverageEngine;
    NtnSim::User::User user("user-0", NtnSim::Utils::GeodeticPosition{0.0, 0.0, 0.0});
    auto userEcef = NtnSim::Utils::MathUtils::GeodeticToEcef(user.GetPosition());

    // Sat 1: directly overhead (90 deg)
    NtnSim::Orbit::OrbitState s1;
    s1.SetEcefPosition(NtnSim::Utils::Position3D{userEcef.x + 600000.0, 0.0, 0.0});
    auto sat1 = std::make_shared<NtnSim::Orbit::Satellite>("sat-overhead", s1);

    // Sat 2: lower angle (around 30-40 deg)
    NtnSim::Orbit::OrbitState s2;
    s2.SetEcefPosition(NtnSim::Utils::Position3D{userEcef.x + 600000.0, 600000.0, 0.0});
    auto sat2 = std::make_shared<NtnSim::Orbit::Satellite>("sat-lower", s2);

    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> sats = {sat2, sat1};

    auto best = coverageEngine.GetBestSatellite(user, sats);
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->GetId(), "sat-overhead");

    // When no satellite is visible (all below horizon)
    NtnSim::Orbit::OrbitState sBelow;
    sBelow.SetEcefPosition(NtnSim::Utils::Position3D{-userEcef.x, 0.0, 0.0}); // other side of Earth
    auto satBelow = std::make_shared<NtnSim::Orbit::Satellite>("sat-below", sBelow);
    std::vector<std::shared_ptr<NtnSim::Orbit::Satellite>> hiddenSats = {satBelow};

    EXPECT_EQ(coverageEngine.GetBestSatellite(user, hiddenSats), nullptr);
}

TEST(CoverageEngineTest, CoverageConfig_CustomMinElevationThreshold) {
    NtnSim::Coverage::CoverageEngine engine;
    EXPECT_DOUBLE_EQ(engine.GetConfig().minElevationDeg, 10.0);

    NtnSim::User::User user("user-0", NtnSim::Utils::GeodeticPosition{0.0, 0.0, 0.0});
    auto userEcef = NtnSim::Utils::MathUtils::GeodeticToEcef(user.GetPosition());

    // Place satellite at an elevation around ~15 degrees
    NtnSim::Orbit::OrbitState state;
    state.SetEcefPosition(NtnSim::Utils::Position3D{userEcef.x + 400000.0, 1500000.0, 0.0});
    NtnSim::Orbit::Satellite sat("sat-mid", state);

    auto info10 = engine.CalculateCoverage(sat, user);
    double elev = info10.GetElevationAngleDeg();
    ASSERT_GT(elev, 10.0);
    ASSERT_LT(elev, 25.0);
    EXPECT_TRUE(info10.IsVisible()); // Visible with minElevation = 10 deg

    // Raise threshold to 30 deg -> now it should be marked not visible
    NtnSim::Coverage::CoverageConfig strictConfig{30.0};
    engine.SetConfig(strictConfig);
    EXPECT_DOUBLE_EQ(engine.GetConfig().minElevationDeg, 30.0);

    auto info30 = engine.CalculateCoverage(sat, user);
    EXPECT_FALSE(info30.IsVisible());
}


