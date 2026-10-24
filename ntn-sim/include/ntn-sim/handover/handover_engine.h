#pragma once

#include "ntn-sim/handover/i_handover_algorithm.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace NtnSim::Metrics {
class MetricsEngine;
}

namespace NtnSim::Handover {

/**
 * @brief Coordinates handover policies, triggering, executions, and telemetry across the constellation.
 */
class HandoverEngine {
public:
    HandoverEngine();
    explicit HandoverEngine(std::unique_ptr<IHandoverAlgorithm> algorithm);
    ~HandoverEngine();

    /**
     * @brief Injects or switches the handover strategy algorithm at runtime.
     */
    void SetAlgorithm(std::unique_ptr<IHandoverAlgorithm> algorithm);

    /**
     * @brief Checks if a handover decision is required for a user, invoking the active strategy.
     * @return Target satellite ID (or empty/current ID if no switch is needed).
     */
    std::string ProcessHandover(
        const User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds = 0.0);

    /**
     * @brief Executes a handover for a user, updating user state and recording telemetry in MetricsEngine.
     * @param user User terminal to switch.
     * @param targetSatId Selected target satellite ID.
     * @param currentTimeSeconds Simulation timestamp.
     * @param interruptionSeconds Switching interruption downtime (default 35 ms).
     * @return True if a handover switch was executed, false otherwise.
     */
    bool ExecuteHandover(
        User::User& user,
        const std::string& targetSatId,
        double currentTimeSeconds,
        double interruptionSeconds = 0.035);

    /**
     * @brief Evaluates candidates and automatically executes handover if target differs from serving.
     */
    bool EvaluateAndExecuteHandover(
        User::User& user,
        const std::vector<std::shared_ptr<Orbit::Satellite>>& candidates,
        const Coverage::CoverageEngine& coverageEngine,
        const Signal::SignalEngine& signalEngine,
        double currentTimeSeconds,
        double interruptionSeconds = 0.035);

    // =========================================================================
    // Metrics Engine Telemetry Hooks
    // =========================================================================

    void SetMetricsEngine(std::shared_ptr<Metrics::MetricsEngine> metricsEngine);
    void SetMetricsEngine(Metrics::MetricsEngine* metricsEngine);
    Metrics::MetricsEngine* GetMetricsEngine() const;

    // Configuration & Telemetry
    void SetDefaultInterruptionTimeSeconds(double seconds);
    double GetDefaultInterruptionTimeSeconds() const;
    size_t GetTotalHandoversExecuted() const;
    void Reset();

private:
    std::unique_ptr<IHandoverAlgorithm> m_algorithm;

    Metrics::MetricsEngine* m_metricsEngineRaw{nullptr};
    std::shared_ptr<Metrics::MetricsEngine> m_metricsEngineShared{nullptr};

    double m_defaultInterruptionSeconds{0.035}; // 35 ms (3GPP NTN beam switch)
    size_t m_totalHandoversExecuted{0};

    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Handover
