#include "ntn-sim/handover/future_ai_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"

namespace NtnSim::Handover {

FutureAIAlgorithm::FutureAIAlgorithm() {}

FutureAIAlgorithm::~FutureAIAlgorithm() {}

std::string FutureAIAlgorithm::MakeDecision(
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
    return candidates.front()->GetId();
}

} // namespace NtnSim::Handover
