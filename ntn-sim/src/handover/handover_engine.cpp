#include "ntn-sim/handover/handover_engine.h"
#include "ntn-sim/handover/best_signal_algorithm.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/metrics/metrics_engine.h"

namespace NtnSim::Handover {

HandoverEngine::HandoverEngine()
    : m_algorithm(std::make_unique<BestSignalAlgorithm>()) {}

HandoverEngine::HandoverEngine(std::unique_ptr<IHandoverAlgorithm> algorithm)
    : m_algorithm(std::move(algorithm)) {}

HandoverEngine::~HandoverEngine() {}

void HandoverEngine::SetAlgorithm(std::unique_ptr<IHandoverAlgorithm> algorithm) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_algorithm = std::move(algorithm);
}

std::string HandoverEngine::ProcessHandover(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine,
    double currentTimeSeconds) {

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_algorithm) {
        return m_algorithm->MakeDecision(user, candidates, coverageEngine, signalEngine, currentTimeSeconds);
    }
    return "";
}

bool HandoverEngine::ExecuteHandover(
    User::User& user,
    const std::string& targetSatId,
    double currentTimeSeconds,
    double interruptionSeconds) {

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string oldSatId = user.GetServingSatelliteId();

    // Case 1: Target is empty (Complete loss of satellite coverage / radio link failure)
    if (targetSatId.empty()) {
        if (!oldSatId.empty()) {
            user.SetServingSatelliteId("");
            if (m_metricsEngineRaw) {
                // Record failed handover
                m_metricsEngineRaw->RecordHandover(user.GetId(), oldSatId, "",
                                                   currentTimeSeconds, false, 0.0);
            }
        }
        return false;
    }

    // Case 2: Target is identical to current serving satellite -> No handover needed
    if (targetSatId == oldSatId) {
        return false;
    }

    // Case 3: Successful Handover execution
    user.SetServingSatelliteId(targetSatId);
    m_totalHandoversExecuted++;

    // Hook to MetricsEngine for 3GPP KPI analytics (ping-pong, interruption time)
    if (m_metricsEngineRaw) {
        m_metricsEngineRaw->RecordHandover(user.GetId(), oldSatId, targetSatId,
                                           currentTimeSeconds, true, interruptionSeconds);
    }

    return true;
}

bool HandoverEngine::EvaluateAndExecuteHandover(
    User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine,
    double currentTimeSeconds,
    double interruptionSeconds) {

    std::string targetSatId = ProcessHandover(user, candidates, coverageEngine, signalEngine, currentTimeSeconds);
    return ExecuteHandover(user, targetSatId, currentTimeSeconds, interruptionSeconds);
}

void HandoverEngine::SetMetricsEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricsEngineShared = std::move(metricsEngine);
    m_metricsEngineRaw = m_metricsEngineShared.get();
}

void HandoverEngine::SetMetricsEngine(Metrics::MetricsEngine* metricsEngine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricsEngineShared.reset();
    m_metricsEngineRaw = metricsEngine;
}

Metrics::MetricsEngine* HandoverEngine::GetMetricsEngine() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metricsEngineRaw;
}

void HandoverEngine::SetDefaultInterruptionTimeSeconds(double seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultInterruptionSeconds = seconds;
}

double HandoverEngine::GetDefaultInterruptionTimeSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_defaultInterruptionSeconds;
}

size_t HandoverEngine::GetTotalHandoversExecuted() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalHandoversExecuted;
}

void HandoverEngine::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_algorithm) {
        m_algorithm->Reset();
    }
    m_totalHandoversExecuted = 0;
}

} // namespace NtnSim::Handover
