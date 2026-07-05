#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"

namespace NtnSim::Handover {

/**
 * @brief Handover policy implementing margin hysteresis and Time-to-Trigger logic.
 */
class HysteresisAlgorithm : public IHandoverAlgorithm {
public:
    HysteresisAlgorithm();
    ~HysteresisAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine) override;
};

} // namespace NtnSim::Handover
