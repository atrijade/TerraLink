#include "ntn-sim/handover/hysteresis_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include <limits>

namespace NtnSim::Handover {

HysteresisAlgorithm::HysteresisAlgorithm(double hysteresisDb, double timeToTriggerSeconds, double carrierFrequencyGhz)
    : m_hysteresisDb(hysteresisDb),
      m_timeToTriggerSeconds(timeToTriggerSeconds),
      m_carrierFrequencyGhz(carrierFrequencyGhz) {}

HysteresisAlgorithm::~HysteresisAlgorithm() {}

void HysteresisAlgorithm::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeTriggers.clear();
}

double HysteresisAlgorithm::GetHysteresisDb() const {
    return m_hysteresisDb;
}

void HysteresisAlgorithm::SetHysteresisDb(double marginDb) {
    m_hysteresisDb = marginDb;
}

double HysteresisAlgorithm::GetTimeToTriggerSeconds() const {
    return m_timeToTriggerSeconds;
}

void HysteresisAlgorithm::SetTimeToTriggerSeconds(double tttSeconds) {
    m_timeToTriggerSeconds = tttSeconds;
}

double HysteresisAlgorithm::GetCarrierFrequencyGhz() const {
    return m_carrierFrequencyGhz;
}

void HysteresisAlgorithm::SetCarrierFrequencyGhz(double freqGhz) {
    m_carrierFrequencyGhz = freqGhz;
}

bool HysteresisAlgorithm::IsTriggerActive(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeTriggers.find(userId) != m_activeTriggers.end();
}

std::string HysteresisAlgorithm::MakeDecision(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine,
    double currentTimeSeconds) {

    if (candidates.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTriggers.erase(user.GetId());
        return "";
    }

    std::string servingSatId = user.GetServingSatelliteId();
    std::shared_ptr<Orbit::Satellite> servingSat = nullptr;
    double servingRsrpDbm = -std::numeric_limits<double>::infinity();

    std::shared_ptr<Orbit::Satellite> bestCandidate = nullptr;
    double bestRsrpDbm = -std::numeric_limits<double>::infinity();
    double bestElevationDeg = -90.0;

    // Evaluate signal measurements across all candidates
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
        double elev = cov.GetElevationAngleDeg();

        if (!servingSatId.empty() && sat->GetId() == servingSatId) {
            servingSat = sat;
            servingRsrpDbm = rsrp;
        }

        if (rsrp > bestRsrpDbm || (rsrp == bestRsrpDbm && elev > bestElevationDeg)) {
            bestRsrpDbm = rsrp;
            bestElevationDeg = elev;
            bestCandidate = sat;
        }
    }

    if (!bestCandidate) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTriggers.erase(user.GetId());
        return "";
    }

    // Case 1: Initial access (User not connected yet) -> Connect immediately
    if (servingSatId.empty()) {
        return bestCandidate->GetId();
    }

    // Case 2: Serving satellite lost line-of-sight / dropped below horizon -> Immediate recovery handover
    if (!servingSat) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTriggers.erase(user.GetId());
        return bestCandidate->GetId();
    }

    // Case 3: Best candidate is already the serving satellite
    if (bestCandidate->GetId() == servingSatId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeTriggers.erase(user.GetId());
        return servingSatId;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Case 4: 3GPP Event A3 evaluation: RSRP_candidate > RSRP_serving + HysteresisMargin
    if (bestRsrpDbm > (servingRsrpDbm + m_hysteresisDb)) {
        auto it = m_activeTriggers.find(user.GetId());
        if (it == m_activeTriggers.end() || it->second.candidateId != bestCandidate->GetId()) {
            // Condition entered for the first time: Start Time-To-Trigger (TTT) timer
            m_activeTriggers[user.GetId()] = A3TriggerRecord{bestCandidate->GetId(), currentTimeSeconds};
            return servingSatId; // Retain serving satellite while TTT is running
        }

        // Candidate has continuously met the margin condition
        double elapsed = currentTimeSeconds - it->second.firstTriggerTime;
        if (elapsed >= m_timeToTriggerSeconds) {
            // TTT timer expired! Trigger handover
            m_activeTriggers.erase(user.GetId());
            return bestCandidate->GetId();
        }

        // Still waiting for TTT duration to elapse
        return servingSatId;
    }

    // Candidate signal dropped below the required hysteresis margin -> Cancel timer
    m_activeTriggers.erase(user.GetId());
    return servingSatId;
}

} // namespace NtnSim::Handover
