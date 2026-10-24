#include "ntn-sim/handover/best_signal_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include <limits>

namespace NtnSim::Handover {

BestSignalAlgorithm::BestSignalAlgorithm(double carrierFrequencyGhz)
    : m_carrierFrequencyGhz(carrierFrequencyGhz) {}

BestSignalAlgorithm::~BestSignalAlgorithm() {}

double BestSignalAlgorithm::GetCarrierFrequencyGhz() const {
    return m_carrierFrequencyGhz;
}

void BestSignalAlgorithm::SetCarrierFrequencyGhz(double freqGhz) {
    m_carrierFrequencyGhz = freqGhz;
}

std::string BestSignalAlgorithm::MakeDecision(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine,
    double currentTimeSeconds) {

    (void)currentTimeSeconds;

    // No visible satellites in sky
    if (candidates.empty()) {
        return "";
    }

    std::string servingSatId = user.GetServingSatelliteId();
    std::shared_ptr<Orbit::Satellite> servingSat = nullptr;
    double servingRsrpDbm = -std::numeric_limits<double>::infinity();

    std::shared_ptr<Orbit::Satellite> bestCandidate = nullptr;
    double bestRsrpDbm = -std::numeric_limits<double>::infinity();
    double bestElevationDeg = -90.0;

    // Evaluate RSRP and elevation angle for every visible candidate
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

        // Check if this candidate is the current serving satellite
        if (!servingSatId.empty() && sat->GetId() == servingSatId) {
            servingSat = sat;
            servingRsrpDbm = rsrp;
        }

        // Select satellite with highest RSRP (and highest elevation as tie-breaker)
        if (rsrp > bestRsrpDbm || (rsrp == bestRsrpDbm && elev > bestElevationDeg)) {
            bestRsrpDbm = rsrp;
            bestElevationDeg = elev;
            bestCandidate = sat;
        }
    }

    if (!bestCandidate) {
        return "";
    }

    // Case 1: Initial access (User is not yet connected to any satellite)
    if (servingSatId.empty()) {
        return bestCandidate->GetId();
    }

    // Case 2: Serving satellite is no longer visible (Radio Link Loss) -> Forced Handover
    if (!servingSat) {
        return bestCandidate->GetId();
    }

    // Case 3: Greedy switch if a candidate offers higher RSRP than current serving satellite
    if (bestRsrpDbm > servingRsrpDbm) {
        return bestCandidate->GetId();
    }

    // Otherwise, retain current serving satellite
    return servingSatId;
}

} // namespace NtnSim::Handover
