#include "ntn-sim/handover/load_aware_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include <algorithm>
#include <limits>

namespace NtnSim::Handover {

LoadAwareAlgorithm::LoadAwareAlgorithm(size_t maxCapacityPerSatellite, double rfWeight, double loadWeight, double carrierFrequencyGhz)
    : m_maxCapacityPerSat(maxCapacityPerSatellite),
      m_rfWeight(rfWeight),
      m_loadWeight(loadWeight),
      m_carrierFrequencyGhz(carrierFrequencyGhz) {}

LoadAwareAlgorithm::~LoadAwareAlgorithm() {}

void LoadAwareAlgorithm::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_satLoads.clear();
}

void LoadAwareAlgorithm::SetSatelliteLoad(const std::string& satId, size_t load) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_satLoads[satId] = load;
}

size_t LoadAwareAlgorithm::GetSatelliteLoad(const std::string& satId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_satLoads.find(satId);
    if (it != m_satLoads.end()) {
        return it->second;
    }
    return 0;
}

void LoadAwareAlgorithm::IncrementSatelliteLoad(const std::string& satId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_satLoads[satId]++;
}

void LoadAwareAlgorithm::DecrementSatelliteLoad(const std::string& satId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_satLoads.find(satId);
    if (it != m_satLoads.end() && it->second > 0) {
        it->second--;
    }
}

void LoadAwareAlgorithm::SetMaxCapacityPerSatellite(size_t maxCap) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxCapacityPerSat = maxCap;
}

size_t LoadAwareAlgorithm::GetMaxCapacityPerSatellite() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxCapacityPerSat;
}

void LoadAwareAlgorithm::SetWeights(double rfWeight, double loadWeight) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rfWeight = rfWeight;
    m_loadWeight = loadWeight;
}

double LoadAwareAlgorithm::GetRfWeight() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rfWeight;
}

double LoadAwareAlgorithm::GetLoadWeight() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loadWeight;
}

std::string LoadAwareAlgorithm::MakeDecision(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine,
    double currentTimeSeconds) {

    (void)currentTimeSeconds;

    if (candidates.empty()) {
        return "";
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string servingSatId = user.GetServingSatelliteId();
    std::shared_ptr<Orbit::Satellite> bestCandidate = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (const auto& sat : candidates) {
        if (!sat) {
            continue;
        }

        auto cov = coverageEngine.CalculateCoverage(*sat, user);
        if (!cov.IsVisible()) {
            continue;
        }

        auto sig = signalEngine.CalculateSignal(cov, m_carrierFrequencyGhz);
        double rsrp = sig.GetRsrpDbm();

        // Normalize RSRP: Range [-130 dBm, -70 dBm] -> [0.0, 1.0]
        double rfNorm = std::clamp((rsrp - (-130.0)) / ((-70.0) - (-130.0)), 0.0, 1.0);

        // Calculate load penalty
        size_t currentLoad = 0;
        auto loadIt = m_satLoads.find(sat->GetId());
        if (loadIt != m_satLoads.end()) {
            currentLoad = loadIt->second;
        }

        double loadRatio = (m_maxCapacityPerSat > 0)
            ? (static_cast<double>(currentLoad) / static_cast<double>(m_maxCapacityPerSat))
            : 0.0;

        // If satellite is at 100% capacity, impose heavy penalty
        double penalty = (loadRatio >= 1.0) ? 1.0 : loadRatio;

        // Composite utility score
        double score = (m_rfWeight * rfNorm) - (m_loadWeight * penalty);

        if (score > bestScore) {
            bestScore = score;
            bestCandidate = sat;
        }
    }

    if (bestCandidate) {
        return bestCandidate->GetId();
    }

    // Fallback to first visible candidate if all scored similarly
    return candidates.front()->GetId();
}

} // namespace NtnSim::Handover
