#pragma once

#include <string>
#include <vector>
#include <memory>

namespace NtnSim::Orbit {
class Satellite;
}
namespace NtnSim::User {
class User;
}
namespace NtnSim::Coverage {
class CoverageEngine;
}
namespace NtnSim::Signal {
class SignalEngine;
}

namespace NtnSim::Handover {

/**
 * @brief Abstract interface for satellite handover decision policies.
 *        Follows the Strategy Pattern to allow dynamic switching between
 *        algorithms (Best-Signal, Hysteresis/TTT, Load-Aware, Trajectory-Predictive).
 */
class IHandoverAlgorithm {
public:
    virtual ~IHandoverAlgorithm() = default;

    /**
     * @brief Computes which satellite the user should connect to or switch to.
     * @param user User terminal under evaluation.
     * @param candidates Set of visible candidate satellites from CoverageEngine.
     * @param coverageEngine Reference to CoverageEngine for spatial geometry and elevation.
     * @param signalEngine Reference to SignalEngine for 3GPP RSRP and SINR metrics.
     * @param currentTimeSeconds Current simulation timestamp (used for Time-to-Trigger timers).
     * @return String satellite ID of target satellite, or empty string / current ID if no switch is needed.
     */
    virtual std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds = 0.0) = 0;

    /**
     * @brief Resets internal tracking state (e.g. active TTT timers).
     */
    virtual void Reset() {}
};

} // namespace NtnSim::Handover
