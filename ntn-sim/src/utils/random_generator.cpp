#include "ntn-sim/utils/random_generator.h"

namespace NtnSim::Utils {

RandomGenerator::RandomGenerator() : m_engine(std::random_device{}()) {}

RandomGenerator::RandomGenerator(unsigned int seed) : m_engine(seed) {}

RandomGenerator::~RandomGenerator() {}

double RandomGenerator::GetUniformDouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(m_engine);
}

int RandomGenerator::GetUniformInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_engine);
}

double RandomGenerator::GetNormalDouble(double mean, double stddev) {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(m_engine);
}

} // namespace NtnSim::Utils
