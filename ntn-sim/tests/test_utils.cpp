#include <gtest/gtest.h>
#include "ntn-sim/utils/id_generator.h"
#include "ntn-sim/utils/math_utils.h"
#include "ntn-sim/utils/random_generator.h"
#include "ntn-sim/utils/time_utils.h"

TEST(UtilsTest, IdGeneratorIncremental) {
    NtnSim::Utils::IdGenerator gen;
    EXPECT_EQ(gen.GetNextId(), 1u);
    EXPECT_EQ(gen.GetNextId(), 2u);
    EXPECT_EQ(gen.GetNextStringId("sat-"), "sat-3");
}

TEST(UtilsTest, MathUtilsCalculations) {
    NtnSim::Utils::Position3D p1{0.0, 0.0, 0.0};
    NtnSim::Utils::Position3D p2{10.0, 20.0, 30.0};
    
    // sqrt(10^2 + 20^2 + 30^2) = sqrt(1400) ≈ 37.4165738677
    EXPECT_NEAR(NtnSim::Utils::MathUtils::CalculateDistance(p1, p2), 37.41657, 1e-4);
    EXPECT_DOUBLE_EQ(NtnSim::Utils::MathUtils::CalculateElevationAngle(p1, p2), 90.0);
}

TEST(UtilsTest, EcefToGeodeticRoundtrip) {
    NtnSim::Utils::GeodeticPosition orig{37.7749, -122.4194, 600000.0};
    auto ecef = NtnSim::Utils::MathUtils::GeodeticToEcef(orig);
    auto geo = NtnSim::Utils::MathUtils::EcefToGeodetic(ecef);

    EXPECT_NEAR(geo.latitude, orig.latitude, 1e-4);
    EXPECT_NEAR(geo.longitude, orig.longitude, 1e-4);
    EXPECT_NEAR(geo.altitude, orig.altitude, 1.0);
}

TEST(UtilsTest, RandomGeneratorInit) {
    NtnSim::Utils::RandomGenerator gen(12345);
    double val = gen.GetUniformDouble(0.0, 10.0);
    EXPECT_GE(val, 0.0);
    EXPECT_LE(val, 10.0);
}

TEST(UtilsTest, TimeUtilsSystemClockStr) {
    EXPECT_FALSE(NtnSim::Utils::TimeUtils::GetSystemTimeString().empty());
}
