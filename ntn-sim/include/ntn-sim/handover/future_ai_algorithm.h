#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"

namespace NtnSim::Handover {

/**
 * @brief Placeholder for future machine learning/AI predictive handover algorithm.
 */
class FutureAIAlgorithm : public IHandoverAlgorithm {
public:
    FutureAIAlgorithm();
    ~FutureAIAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine) override;
};

} // namespace NtnSim::Handover
