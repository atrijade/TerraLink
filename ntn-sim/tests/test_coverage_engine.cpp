#include <gtest/gtest.h>
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"

TEST(CoverageEngineTest, CalculateCoverageSkeleton) {
    NtnSim::Coverage::CoverageEngine engine;
    
    NtnSim::Orbit::OrbitState satState;
    NtnSim::Orbit::Satellite sat("sat-1", satState);
    NtnSim::Utils::GeodeticPosition userPos{0.0, 0.0, 0.0};
    NtnSim::User::User user("user-1", userPos);
    
    auto info = engine.CalculateCoverage(sat, user);
    EXPECT_TRUE(info.IsVisible());
    EXPECT_DOUBLE_EQ(info.GetDistanceMeters(), 1000000.0);
    EXPECT_DOUBLE_EQ(info.GetElevationAngleDeg(), 45.0);
}
