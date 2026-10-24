#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"

namespace NtnSim::Handover {

/**
 * @brief Trajectory-Predictive Handover Algorithm.
 * 
 * Uses Keplerian geometry and Line-of-Sight (LoS) velocity to determine whether
 * candidate satellites are rising (approaching) or setting (receding).
 * Prioritizes rising satellites offering the longest remaining service duration,
 * minimizing total handover frequency across the constellation.
 */
class FutureAIAlgorithm : public IHandoverAlgorithm {
public:
    explicit FutureAIAlgorithm(double carrierFrequencyGhz = 2.0, double trajectoryWeight = 0.5);
    ~FutureAIAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds = 0.0) override;

    double GetTrajectoryWeight() const;
    void SetTrajectoryWeight(double weight);

private:
    double m_carrierFrequencyGhz{2.0};
    double m_trajectoryWeight{0.5}; // Weight given to rising vs setting trajectory advantage
};

} // namespace NtnSim::Handover
