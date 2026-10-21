#include <gtest/gtest.h>
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include <cmath>

TEST(OrbitEngineTest, AddAndRetrieveSatellite) {
    NtnSim::Orbit::OrbitEngine engine;
    
    NtnSim::Orbit::OrbitState state;
    NtnSim::Utils::Position3D pos{100.0, 200.0, 300.0};
    state.SetEcefPosition(pos);
    
    auto sat = std::make_shared<NtnSim::Orbit::Satellite>("sat-1", state);
    engine.AddSatellite(sat);
    
    auto retrieved = engine.GetSatellite("sat-1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->GetId(), "sat-1");
    EXPECT_DOUBLE_EQ(retrieved->GetOrbitState().GetEcefPosition().x, 100.0);
    
    EXPECT_EQ(engine.GetSatellite("non-existent"), nullptr);
    EXPECT_EQ(engine.GetAllSatellites().size(), 1);
}

TEST(OrbitEngineTest, PropagationKeplerianMotionCircular) {
    // 600 km altitude, 0 deg inclination (equatorial), 0 deg RAAN, 0 deg ArgPerigee, 0 deg initial M0
    NtnSim::Orbit::OrbitState state(600.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    
    double period = state.GetOrbitalPeriodSeconds();
    // Expected period for a = 6978.137 km is ~5800.7 seconds (~96.7 minutes)
    EXPECT_NEAR(period, 5800.7, 5.0);
    EXPECT_NEAR(state.GetVelocityKms(), 7.558, 0.01);
    
    // At t = 0
    state.Propagate(0.0);
    auto p0 = state.GetOrbitalPlanePosition();
    EXPECT_NEAR(p0.x, 6978137.0, 1.0);
    EXPECT_NEAR(p0.y, 0.0, 1.0);
    EXPECT_NEAR(p0.z, 0.0, 1.0);
    EXPECT_NEAR(state.GetCurrentMeanAnomalyDeg(), 0.0, 0.01);

    // At quarter orbit: t = T / 4
    state.Propagate(period / 4.0);
    auto p_q = state.GetOrbitalPlanePosition();
    EXPECT_NEAR(p_q.x, 0.0, 1.0);
    EXPECT_NEAR(p_q.y, 6978137.0, 1.0);
    EXPECT_NEAR(state.GetCurrentMeanAnomalyDeg(), 90.0, 0.01);
    EXPECT_NEAR(state.GetCurrentTrueAnomalyDeg(), 90.0, 0.01);

    // At half orbit: t = T / 2
    state.Propagate(period / 2.0);
    auto p_h = state.GetOrbitalPlanePosition();
    EXPECT_NEAR(p_h.x, -6978137.0, 1.0);
    EXPECT_NEAR(p_h.y, 0.0, 1.0);
    EXPECT_NEAR(state.GetCurrentMeanAnomalyDeg(), 180.0, 0.01);

    // At full orbit: t = T
    state.Propagate(period);
    auto p_full = state.GetOrbitalPlanePosition();
    EXPECT_NEAR(p_full.x, 6978137.0, 1.0);
    EXPECT_NEAR(p_full.y, 0.0, 1.0);
    EXPECT_NEAR(state.GetCurrentMeanAnomalyDeg(), 0.0, 0.01);
}

TEST(OrbitEngineTest, PropagationPolarOrbitECI) {
    // 600 km altitude, 90 deg inclination (polar orbit), 0 deg RAAN, 0 deg ArgPerigee
    NtnSim::Orbit::OrbitState state(600.0, 90.0, 0.0, 0.0, 0.0, 0.0);
    double period = state.GetOrbitalPeriodSeconds();

    // At t = 0: Satellite crosses equator in X-Z plane
    state.Propagate(0.0);
    auto eci0 = state.GetEciPosition();
    EXPECT_NEAR(eci0.x, 6978137.0, 1.0);
    EXPECT_NEAR(eci0.y, 0.0, 1.0);
    EXPECT_NEAR(eci0.z, 0.0, 1.0);

    // At t = T / 4: Satellite passes over North Pole (Z axis)
    state.Propagate(period / 4.0);
    auto eci_north = state.GetEciPosition();
    EXPECT_NEAR(eci_north.x, 0.0, 1.0);
    EXPECT_NEAR(eci_north.y, 0.0, 1.0);
    EXPECT_NEAR(eci_north.z, 6978137.0, 1.0);
}

TEST(OrbitEngineTest, PropagationEccentricOrbit) {
    // Semi-major axis = 7000 km, e = 0.1, inc = 0 deg
    double altitudeKm = 7000.0 - 6378.137;
    NtnSim::Orbit::OrbitState state(altitudeKm, 0.0, 0.0, 0.0, 0.0, 0.1);

    // At periapsis (M = 0): r = a * (1 - e) = 7000 * 0.9 = 6300 km = 6,300,000 m
    state.Propagate(0.0);
    auto p_peri = state.GetOrbitalPlanePosition();
    EXPECT_NEAR(p_peri.x, 6300000.0, 1.0);
    EXPECT_NEAR(p_peri.y, 0.0, 1.0);

    // Velocity at periapsis should be higher than mean circular speed
    EXPECT_GT(state.GetVelocityKms(), 7.55);
}

TEST(OrbitEngineTest, OrbitEnginePropagatesAllSatellites) {
    NtnSim::Orbit::OrbitEngine engine;
    NtnSim::Orbit::OrbitState state(600.0, 87.5, 0.0, 0.0, 0.0, 0.0);
    auto sat = std::make_shared<NtnSim::Orbit::Satellite>("sat-leo-1", state);
    engine.AddSatellite(sat);

    // Advance simulation time by 500 seconds
    engine.UpdatePositions(500.0);

    auto retrieved = engine.GetSatellite("sat-leo-1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_GT(retrieved->GetOrbitState().GetCurrentMeanAnomalyDeg(), 0.0);
}

