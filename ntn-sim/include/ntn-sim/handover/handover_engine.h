#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"
#include <memory>
#include <mutex>

namespace NtnSim::Handover {

/**
 * @brief Coordinates handover policies, triggering, and executions across users.
 */
class HandoverEngine {
public:
    HandoverEngine();
    ~HandoverEngine();

    /**
     * @brief Injects the strategy algorithm.
     */
    void SetAlgorithm(std::unique_ptr<IHandoverAlgorithm> algorithm);

    /**
     * @brief Checks if a handover decision is required for a user, invoking the strategy.
     */
    std::string ProcessHandover(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine);

private:
    std::unique_ptr<IHandoverAlgorithm> m_algorithm;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Handover
