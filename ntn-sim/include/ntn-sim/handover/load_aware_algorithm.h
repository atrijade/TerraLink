#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"

namespace NtnSim::Handover {

/**
 * @brief Handover policy factoring satellite congestion into decision.
 */
class LoadAwareAlgorithm : public IHandoverAlgorithm {
public:
    LoadAwareAlgorithm();
    ~LoadAwareAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine) override;
};

} // namespace NtnSim::Handover
