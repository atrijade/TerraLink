#include "ntn-sim/utils/random_generator.h"

namespace NtnSim::Utils {

RandomGenerator::RandomGenerator() : m_engine(std::random_device{}()) {}

RandomGenerator::RandomGenerator(unsigned int seed) : m_engine(seed) {}

RandomGenerator::~RandomGenerator() {}

double RandomGenerator::GetUniformDouble(double min, double max) {
    (void)min;
    (void)max;
    // Skeleton implementation
    return 0.0;
}

int RandomGenerator::GetUniformInt(int min, int max) {
    (void)min;
    (void)max;
    // Skeleton implementation
    return 0;
}

double RandomGenerator::GetNormalDouble(double mean, double stddev) {
    (void)mean;
    (void)stddev;
    // Skeleton implementation
    return 0.0;
}

} // namespace NtnSim::Utils
