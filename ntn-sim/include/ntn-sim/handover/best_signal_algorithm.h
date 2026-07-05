#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"

namespace NtnSim::Handover {

/**
 * @brief Handover policy choosing satellite with strongest RSRP.
 */
class BestSignalAlgorithm : public IHandoverAlgorithm {
public:
    BestSignalAlgorithm();
    ~BestSignalAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine) override;
};

} // namespace NtnSim::Handover
