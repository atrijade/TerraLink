#include "ntn-sim/handover/future_ai_algorithm.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/signal/signal_engine.h"
#include <algorithm>
#include <limits>

namespace NtnSim::Handover {

FutureAIAlgorithm::FutureAIAlgorithm(double carrierFrequencyGhz, double trajectoryWeight)
    : m_carrierFrequencyGhz(carrierFrequencyGhz),
      m_trajectoryWeight(trajectoryWeight) {}

FutureAIAlgorithm::~FutureAIAlgorithm() {}

double FutureAIAlgorithm::GetTrajectoryWeight() const {
    return m_trajectoryWeight;
}

void FutureAIAlgorithm::SetTrajectoryWeight(double weight) {
    m_trajectoryWeight = weight;
}

std::string FutureAIAlgorithm::MakeDecision(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
    const Coverage::CoverageEngine& coverageEngine,
    const Signal::SignalEngine& signalEngine,
    double currentTimeSeconds) {

    (void)currentTimeSeconds;

    if (candidates.empty()) {
        return "";
    }

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
        double losVelKms = cov.GetLosVelocityKms(); // >0 = approaching, <0 = receding

        // RF quality score normalized to [0, 1]
        double rfScore = std::clamp((rsrp - (-130.0)) / ((-70.0) - (-130.0)), 0.0, 1.0);

        // Trajectory bonus: approaching satellites (+losVel) have longer visible service life
        // Max LEO speed ~7.5 km/s -> normalize losVelKms / 7.5 to [-1.0, 1.0]
        double trajectoryScore = std::clamp(losVelKms / 7.5, -1.0, 1.0);

        double totalScore = (1.0 - m_trajectoryWeight) * rfScore + m_trajectoryWeight * trajectoryScore;

        if (totalScore > bestScore) {
            bestScore = totalScore;
            bestCandidate = sat;
        }
    }

    if (bestCandidate) {
        return bestCandidate->GetId();
    }

    return candidates.front()->GetId();
}

} // namespace NtnSim::Handover
