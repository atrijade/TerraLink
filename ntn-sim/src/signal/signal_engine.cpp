#include "ntn-sim/signal/signal_engine.h"
#include "ntn-sim/signal/connection.h"
#include "ntn-sim/coverage/coverage_info.h"
#include <cmath>
#include <algorithm>
#include <numbers>

namespace NtnSim::Signal {

SignalEngine::SignalEngine() {}

SignalEngine::SignalEngine(const ChannelConfig& config) : m_config(config) {}

SignalEngine::~SignalEngine() {}

// ---------------------------------------------------------------------------
// Private physics helpers
// ---------------------------------------------------------------------------

double SignalEngine::ComputeFsplDb(double distanceMeters, double frequencyGhz) const {
    // 3GPP TR 38.811 free space path loss:
    // FSPL(dB) = 20*log10(d_m) + 20*log10(f_GHz) + 32.45
    if (distanceMeters <= 0.0 || frequencyGhz <= 0.0) {
        return 300.0;  // Effectively infinite loss
    }
    return 20.0 * std::log10(distanceMeters)
         + 20.0 * std::log10(frequencyGhz)
         + 32.45;
}

double SignalEngine::ComputeAtmosphericLossDb(double elevationDeg) const {
    // Atmospheric attenuation (gas absorption) scales with slant range.
    // At zenith (90 deg) the loss is the minimum zenith value.
    // At lower angles the signal traverses more atmosphere: A(theta) = A_zenith / sin(theta)
    // Clamped at a minimum elevation of 5 deg to avoid divide-by-zero near horizon.
    double elev = std::max(elevationDeg, 5.0);
    double sinElev = std::sin(elev * std::numbers::pi / 180.0);
    return m_config.atmosphericZenithDb / sinElev;
}

double SignalEngine::ComputeNoisePowerDbm() const {
    // N_total (dBm) = -174 dBm/Hz + 10*log10(BW_Hz) + NF_dB
    double bandwidthHz = m_config.bandwidthMhz * 1.0e6;
    return BOLTZMANN_DBM_HZ + 10.0 * std::log10(bandwidthHz) + m_config.noiseFigureDb;
}

double SignalEngine::ComputeDopplerShiftHz(double losVelocityKms, double frequencyGhz) const {
    // Delta_f = f_c * v_los / c
    // v_los in m/s; positive = satellite approaching user (upshift)
    double losVelocityMs = losVelocityKms * 1000.0;
    double frequencyHz   = frequencyGhz * 1.0e9;
    return frequencyHz * losVelocityMs / SPEED_OF_LIGHT_M_S;
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

SignalInfo SignalEngine::CalculateSignal(const Coverage::CoverageInfo& coverage, double frequencyGhz) const {
    SignalInfo info;

    // Out-of-coverage: satellite below horizon or not visible
    if (!coverage.IsVisible() || coverage.GetElevationAngleDeg() < m_config.minElevationDeg) {
        info.SetPathLossDb(300.0);
        info.SetRsrpDbm(-140.0);    // 3GPP out-of-range floor
        info.SetSinrDb(-30.0);
        info.SetLatencySeconds(0.0);
        info.SetDopplerShiftHz(0.0);
        return info;
    }

    double distM     = coverage.GetDistanceMeters();
    double elevDeg   = coverage.GetElevationAngleDeg();
    double losVelKms = coverage.GetLosVelocityKms();

    // ---- Step 1a: Free Space Path Loss ----
    double fsplDb = ComputeFsplDb(distM, frequencyGhz);

    // ---- Step 1b: Elevation-dependent atmospheric absorption ----
    double atmosDb = ComputeAtmosphericLossDb(elevDeg);

    // ---- Total path loss ----
    double totalPathLossDb = fsplDb + atmosDb + m_config.miscLossDb;
    info.SetPathLossDb(totalPathLossDb);

    // ---- Step 1c: Received Signal Reference Power (RSRP) ----
    // EIRP (dBm) = P_tx + G_tx
    double eirpDbm = m_config.txPowerDbm + m_config.txAntennaGainDbi;
    // RSRP (dBm) = EIRP + G_rx - TotalPathLoss
    double rsrpDbm = eirpDbm + m_config.rxAntennaGainDbi - totalPathLossDb;
    info.SetRsrpDbm(rsrpDbm);

    // ---- Step 1d: SINR ----
    // Assuming interference-limited scenario with ~3 dB interference margin
    constexpr double INTERFERENCE_MARGIN_DB = 3.0;
    double noisePowerDbm = ComputeNoisePowerDbm();
    double sinrDb = rsrpDbm - noisePowerDbm - INTERFERENCE_MARGIN_DB;
    info.SetSinrDb(sinrDb);

    // ---- Step 1e: Propagation delay: tau = d / c ----
    double latencySeconds = distM / SPEED_OF_LIGHT_M_S;
    info.SetLatencySeconds(latencySeconds);

    // ---- Step 1f: Doppler shift: Delta_f = f_c * v_los / c ----
    double dopplerHz = ComputeDopplerShiftHz(losVelKms, frequencyGhz);
    info.SetDopplerShiftHz(dopplerHz);

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
        // Mark inactive if out-of-coverage (RSRP below floor threshold)
        it->second->SetActive(info.GetRsrpDbm() > -139.0);
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

ChannelConfig SignalEngine::GetChannelConfig() const {
    return m_config;
}

void SignalEngine::SetChannelConfig(const ChannelConfig& config) {
    m_config = config;
}

} // namespace NtnSim::Signal
