#include <gtest/gtest.h>
#include "ntn-sim/utils/id_generator.h"
#include "ntn-sim/utils/math_utils.h"
#include "ntn-sim/utils/random_generator.h"
#include "ntn-sim/utils/time_utils.h"

TEST(UtilsTest, IdGeneratorIncremental) {
    NtnSim::Utils::IdGenerator gen;
    EXPECT_EQ(gen.GetNextId(), 1);
    EXPECT_EQ(gen.GetNextId(), 2);
    EXPECT_EQ(gen.GetNextStringId("pkt-"), "pkt-3");
}

TEST(UtilsTest, MathUtilsCalculations) {
    NtnSim::Utils::Position3D p1{0.0, 0.0, 0.0};
    NtnSim::Utils::Position3D p2{10.0, 20.0, 30.0};
    
    // sqrt(10^2 + 20^2 + 30^2) = sqrt(1400) ≈ 37.4165738677
    EXPECT_NEAR(NtnSim::Utils::MathUtils::CalculateDistance(p1, p2), 37.41657, 1e-4);
    EXPECT_DOUBLE_EQ(NtnSim::Utils::MathUtils::CalculateElevationAngle(p1, p2), 90.0);
}

TEST(UtilsTest, RandomGeneratorInit) {
    NtnSim::Utils::RandomGenerator gen(12345);
    EXPECT_DOUBLE_EQ(gen.GetUniformDouble(0.0, 10.0), 0.0);
}

TEST(UtilsTest, TimeUtilsSystemClockStr) {
    EXPECT_FALSE(NtnSim::Utils::TimeUtils::GetSystemTimeString().empty());
}
