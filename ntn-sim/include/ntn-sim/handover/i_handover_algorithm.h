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
 */
class IHandoverAlgorithm {
public:
    virtual ~IHandoverAlgorithm() = default;

    /**
     * @brief Computes which satellite the user should switch to.
     * @param user User under evaluation.
     * @param candidates Set of visible candidate satellites.
     * @param coverageEngine Reference to CoverageEngine for geo distances.
     * @param signalEngine Reference to SignalEngine for RSRP/SINR metrics.
     * @return String satellite ID of target, or empty if no handover is required.
     */
    virtual std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine) = 0;
};

} // namespace NtnSim::Handover
