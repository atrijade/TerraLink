#include "ntn-sim/handover/best_signal_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"

namespace NtnSim::Handover {

BestSignalAlgorithm::BestSignalAlgorithm() {}

BestSignalAlgorithm::~BestSignalAlgorithm() {}

std::string BestSignalAlgorithm::MakeDecision(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine) {
    
    (void)user;
    (void)coverageEngine;
    (void)signalEngine;

    if (candidates.empty()) {
        return "";
    }
    // Return first candidate as default skeleton decision
    return candidates.front()->GetId();
}

} // namespace NtnSim::Handover
