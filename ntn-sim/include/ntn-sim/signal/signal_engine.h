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
 * @brief Configurable RF link budget parameters for a satellite downlink channel.
 */
struct ChannelConfig {
    double txPowerDbm{43.0};        ///< Satellite transmit power in dBm
    double txAntennaGainDbi{15.0};  ///< Satellite transmit antenna gain in dBi (EIRP = tx + gain)
    double rxAntennaGainDbi{0.0};   ///< UE receive antenna gain in dBi (0 = handheld, 25 = VSAT dish)
    double noiseFigureDb{7.0};      ///< Receiver noise figure in dB
    double bandwidthMhz{20.0};      ///< Channel bandwidth in MHz
    double miscLossDb{2.0};         ///< Miscellaneous losses: polarization, feeder, body loss (dB)
    double atmosphericZenithDb{0.5};///< Zenith atmospheric gas absorption in dB (3GPP TR 38.811)
    double minElevationDeg{10.0};   ///< Below this angle the link is considered out-of-coverage
};

/**
 * @brief Computes path loss, RSRP, SINR, Doppler offset, and channel latency
 *        based on 3GPP NTN (TR 38.811 / TR 38.821) link budget equations.
 */
class SignalEngine {
public:
    SignalEngine();
    explicit SignalEngine(const ChannelConfig& config);
    ~SignalEngine();

    /**
     * @brief Computes signal attributes based on coverage geometry (3GPP NTN link budget).
     * @param coverage Spatial relationship (distance, elevation, LoS velocity) from CoverageEngine.
     * @param frequencyGhz Carrier frequency in GHz (e.g. 2.1 for S-Band, 20.0 for Ka-Band).
     * @return SignalInfo populated with FSPL, RSRP, SINR, latency, and Doppler shift.
     */
    SignalInfo CalculateSignal(const Coverage::CoverageInfo& coverage, double frequencyGhz) const;

    /**
     * @brief Creates or updates a dynamic channel connection between two endpoints.
     */
    void UpdateConnection(const std::string& srcId, const std::string& destId, const SignalInfo& info);

    /**
     * @brief Fetches an active connection pointer.
     */
    std::shared_ptr<Connection> GetConnection(const std::string& srcId, const std::string& destId) const;

    ChannelConfig GetChannelConfig() const;
    void SetChannelConfig(const ChannelConfig& config);

private:
    // Physical constants
    static constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;
    static constexpr double BOLTZMANN_DBM_HZ   = -174.0;  ///< kT at 290K in dBm/Hz

    /**
     * @brief Computes Free Space Path Loss in dB.
     * @param distanceMeters Distance between satellite and user (m).
     * @param frequencyGhz Carrier frequency (GHz).
     */
    double ComputeFsplDb(double distanceMeters, double frequencyGhz) const;

    /**
     * @brief Computes elevation-dependent atmospheric absorption in dB.
     * @param elevationDeg Elevation angle in degrees.
     */
    double ComputeAtmosphericLossDb(double elevationDeg) const;

    /**
     * @brief Computes total thermal noise power in dBm.
     */
    double ComputeNoisePowerDbm() const;

    /**
     * @brief Computes Doppler shift in Hz.
     * @param losVelocityKms Line-of-sight velocity component in km/s (+ = approaching).
     * @param frequencyGhz Carrier frequency in GHz.
     */
    double ComputeDopplerShiftHz(double losVelocityKms, double frequencyGhz) const;

    ChannelConfig m_config;
    std::unordered_map<std::string, std::shared_ptr<Connection>> m_connections;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Signal
