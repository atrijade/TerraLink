#pragma once

#include <random>

namespace NtnSim::Utils {

/**
 * @brief Thread-safe pseudorandom number generator utility.
 */
class RandomGenerator {
public:
    /**
     * @brief Constructor initializing with a default seed.
     */
    RandomGenerator();

    /**
     * @brief Constructor with custom seed.
     * @param seed Simulation seed value.
     */
    explicit RandomGenerator(unsigned int seed);

    ~RandomGenerator();

    /**
     * @brief Generates a random double in the range [min, max].
     */
    double GetUniformDouble(double min, double max);

    /**
     * @brief Generates a random integer in the range [min, max].
     */
    int GetUniformInt(int min, int max);

    /**
     * @brief Generates a random double following normal distribution.
     */
    double GetNormalDouble(double mean, double stddev);

private:
    std::mt19937 m_engine;
};

} // namespace NtnSim::Utils
