#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"
#include <unordered_map>
#include <mutex>
#include <cstddef>

namespace NtnSim::Handover {

/**
 * @brief Load-Aware Handover policy that balances RF signal quality against satellite congestion.
 * 
 * Computes a composite utility score:
 *   Score = w_rf * NormalizedRSRP - w_load * (CurrentLoad / MaxCapacity)
 * Prevents hot-spot overload by routing users to less congested satellites.
 */
class LoadAwareAlgorithm : public IHandoverAlgorithm {
public:
    explicit LoadAwareAlgorithm(size_t maxCapacityPerSatellite = 100,
                                double rfWeight = 0.7,
                                double loadWeight = 0.3,
                                double carrierFrequencyGhz = 2.0);
    ~LoadAwareAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds = 0.0) override;

    void Reset() override;

    void SetSatelliteLoad(const std::string& satId, size_t load);
    size_t GetSatelliteLoad(const std::string& satId) const;
    void IncrementSatelliteLoad(const std::string& satId);
    void DecrementSatelliteLoad(const std::string& satId);

    void SetMaxCapacityPerSatellite(size_t maxCap);
    size_t GetMaxCapacityPerSatellite() const;

    void SetWeights(double rfWeight, double loadWeight);
    double GetRfWeight() const;
    double GetLoadWeight() const;

private:
    size_t m_maxCapacityPerSat{100};
    double m_rfWeight{0.7};
    double m_loadWeight{0.3};
    double m_carrierFrequencyGhz{2.0};

    std::unordered_map<std::string, size_t> m_satLoads;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Handover
