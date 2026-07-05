#pragma once

#include <string>

namespace NtnSim::Utils {

/**
 * @brief Time conversion and representation utilities.
 */
class TimeUtils {
public:
    TimeUtils() = delete;
    ~TimeUtils() = delete;

    /**
     * @brief Formats a simulation timestamp (seconds) into a HH:MM:SS.ms string.
     * @param simSeconds Simulation seconds.
     * @return Formatted time string.
     */
    static std::string FormatSimTime(double simSeconds);

    /**
     * @brief Obtains the current system wall-clock time formatted string.
     */
    static std::string GetSystemTimeString();
};

} // namespace NtnSim::Utils
