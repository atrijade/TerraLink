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

TEST(UtilsTest, MathUtilsDefaultStubs) {
    NtnSim::Utils::Position3D p1{0.0, 0.0, 0.0};
    NtnSim::Utils::Position3D p2{10.0, 20.0, 30.0};
    
    EXPECT_DOUBLE_EQ(NtnSim::Utils::MathUtils::CalculateDistance(p1, p2), 0.0);
    EXPECT_DOUBLE_EQ(NtnSim::Utils::MathUtils::CalculateElevationAngle(p1, p2), 90.0);
}

TEST(UtilsTest, RandomGeneratorInit) {
    NtnSim::Utils::RandomGenerator gen(12345);
    EXPECT_DOUBLE_EQ(gen.GetUniformDouble(0.0, 10.0), 0.0);
}

TEST(UtilsTest, TimeUtilsSystemClockStr) {
    EXPECT_FALSE(NtnSim::Utils::TimeUtils::GetSystemTimeString().empty());
}
