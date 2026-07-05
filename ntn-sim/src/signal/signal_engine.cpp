#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/signal/connection.h"
#include "ntn-sim/coverage/coverage_info.h"

namespace NtnSim::Signal {

SignalEngine::SignalEngine() {}

SignalEngine::~SignalEngine() {}

SignalInfo SignalEngine::CalculateSignal(const Coverage::CoverageInfo& coverage, double frequencyGhz) {
    (void)coverage;
    (void)frequencyGhz;
    // Skeleton implementation
    SignalInfo info;
    info.SetPathLossDb(130.0);
    info.SetRsrpDbm(-85.0);
    info.SetSinrDb(18.0);
    info.SetLatencySeconds(0.015); // 15 ms
    info.SetDopplerShiftHz(500.0);
    return info;
}

void SignalEngine::UpdateConnection(const std::string& srcId, const std::string& destId, const SignalInfo& info) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string key = srcId + "->" + destId;
    auto it = m_connections.find(key);
    if (it == m_connections.end()) {
        auto conn = std::make_shared<Connection>(srcId, destId);
        conn->SetActive(true);
        conn->SetSignalInfo(info);
        m_connections[key] = conn;
    } else {
        it->second->SetSignalInfo(info);
    }
}

std::shared_ptr<Connection> SignalEngine::GetConnection(const std::string& srcId, const std::string& destId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string key = srcId + "->" + destId;
    auto it = m_connections.find(key);
    if (it != m_connections.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace NtnSim::Signal
