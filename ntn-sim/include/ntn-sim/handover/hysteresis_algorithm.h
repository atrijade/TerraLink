#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"
#include <unordered_map>
#include <mutex>

namespace NtnSim::Handover {

/**
 * @brief Telemetry tracking record for an ongoing 3GPP Event A3 Time-To-Trigger (TTT) window.
 */
struct A3TriggerRecord {
    std::string candidateId;
    double firstTriggerTime{0.0};
};

/**
 * @brief 3GPP Event A3 Handover Algorithm with Hysteresis Margin (dB) and Time-To-Trigger (TTT).
 * 
 * Standardized in 3GPP TS 38.331 / TR 38.821 to prevent the "ping-pong" effect.
 * A candidate satellite must be stronger than the serving satellite by at least
 * `hysteresisDb` for a continuous duration of `timeToTriggerSeconds` before a handover is triggered.
 */
class HysteresisAlgorithm : public IHandoverAlgorithm {
public:
    explicit HysteresisAlgorithm(double hysteresisDb = 3.0,
                                 double timeToTriggerSeconds = 2.0,
                                 double carrierFrequencyGhz = 2.0);
    ~HysteresisAlgorithm() override;

    std::string MakeDecision(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds = 0.0) override;

    void Reset() override;

    double GetHysteresisDb() const;
    void SetHysteresisDb(double marginDb);

    double GetTimeToTriggerSeconds() const;
    void SetTimeToTriggerSeconds(double tttSeconds);

    double GetCarrierFrequencyGhz() const;
    void SetCarrierFrequencyGhz(double freqGhz);

    bool IsTriggerActive(const std::string& userId) const;

private:
    double m_hysteresisDb{3.0};          // 3GPP A3 Handover Margin (default 3.0 dB)
    double m_timeToTriggerSeconds{2.0};   // Time-To-Trigger timer (default 2.0 s)
    double m_carrierFrequencyGhz{2.0};

    // Tracks ongoing TTT timers keyed by User ID
    std::unordered_map<std::string, A3TriggerRecord> m_activeTriggers;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Handover
