#include "ntn-sim/handover/handover_engine.h"
#include "ntn-sim/handover/best_signal_algorithm.h"

namespace NtnSim::Handover {

HandoverEngine::HandoverEngine() : m_algorithm(std::make_unique<BestSignalAlgorithm>()) {}

HandoverEngine::~HandoverEngine() {}

void HandoverEngine::SetAlgorithm(std::unique_ptr<IHandoverAlgorithm> algorithm) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_algorithm = std::move(algorithm);
}

std::string HandoverEngine::ProcessHandover(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine) {
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_algorithm) {
        return m_algorithm->MakeDecision(user, candidates, coverageEngine, signalEngine);
    }
    return "";
}

} // namespace NtnSim::Handover
