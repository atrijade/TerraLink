#pragma once

#include "ntn-sim/signal/signal_info.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace NtnSim::Coverage {
class CoverageInfo;
}

namespace NtnSim::Signal {
class Connection;

/**
 * @brief Computes path loss, RSRP, SINR, Doppler offset, and channel latency.
 */
class SignalEngine {
public:
    SignalEngine();
    ~SignalEngine();

    /**
     * @brief Computes signal attributes based on geodetic metrics.
     * @param coverage Data describing spatial distance and line-of-sight.
     * @param frequencyGhz Carrier frequency in GHz.
     * @return SignalInfo output.
     */
    SignalInfo CalculateSignal(const Coverage::CoverageInfo& coverage, double frequencyGhz);

    /**
     * @brief Creates or updates a dynamic channel connection between two endpoints.
     */
    void UpdateConnection(const std::string& srcId, const std::string& destId, const SignalInfo& info);

    /**
     * @brief Fetches an active connection pointer.
     */
    std::shared_ptr<Connection> GetConnection(const std::string& srcId, const std::string& destId) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Connection>> m_connections;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Signal
