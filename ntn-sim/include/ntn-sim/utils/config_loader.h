#pragma once

#include "ntn-sim/utils/simulation_config.h"
#include <string>

namespace NtnSim::Utils {

/**
 * @brief Utility for loading simulator configuration from files.
 */
class ConfigLoader {
public:
    ConfigLoader() = default;
    ~ConfigLoader() = default;

    /**
     * @brief Loads simulation parameters from a JSON configuration file.
     * @param filepath Path to the json file.
     * @return SimulationConfig loaded structure.
     */
    SimulationConfig LoadFromFile(const std::string& filepath);
};

} // namespace NtnSim::Utils
