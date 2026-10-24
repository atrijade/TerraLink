#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"

namespace NtnSim::Handover {

/**
 * @brief Greedy Handover policy based on instantaneous maximum Reference Signal Received Power (RSRP).
 * 
 * Selects whichever satellite provides the strongest signal or highest elevation.
 * Serves as the standard baseline in 5G NTN benchmarking.
 */
class BestSignalAlgorithm : public IHandoverAlgorithm {
public:
    explicit BestSignalAlgorithm(double carrierFrequencyGhz = 2.0);
    ~BestSignalAlgorithm() override;

    /**
     * @brief Evaluates all visible candidates and returns the satellite with maximum RSRP.
     */
    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds = 0.0) override;

    double GetCarrierFrequencyGhz() const;
    void SetCarrierFrequencyGhz(double freqGhz);

private:
    double m_carrierFrequencyGhz{2.0}; // Default 2.0 GHz S-Band (3GPP NTN)
};

} // namespace NtnSim::Handover
