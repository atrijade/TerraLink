#pragma once

#include <string>
#include <vector>

namespace NtnSim::Utils {

/**
 * @brief Structure representing satellite configuration.
 */
struct SatelliteConfig {
    std::string id;
    double semiMajorAxis{0.0};
    double inclination{0.0};
    double longitudeOfAscendingNode{0.0};
    double anomaly{0.0};
};

/**
 * @brief Structure representing gateway configuration.
 */
struct GatewayConfig {
    std::string id;
    double latitude{0.0};
    double longitude{0.0};
    double altitude{0.0};
};

/**
 * @brief Configuration model for the simulation run.
 */
class SimulationConfig {
public:
    SimulationConfig();
    ~SimulationConfig();

    double GetDurationSeconds() const;
    void SetDurationSeconds(double val);

    double GetTimeStepSeconds() const;
    void SetTimeStepSeconds(double val);

    unsigned int GetSeed() const;
    void SetSeed(unsigned int val);

    const std::vector<SatelliteConfig>& GetSatellites() const;
    void AddSatellite(const SatelliteConfig& sat);

    const std::vector<GatewayConfig>& GetGateways() const;
    void AddGateway(const GatewayConfig& gw);

private:
    double m_durationSeconds{3600.0};
    double m_timeStepSeconds{1.0};
    unsigned int m_seed{42};
    std::vector<SatelliteConfig> m_satellites;
    std::vector<GatewayConfig> m_gateways;
};

} // namespace NtnSim::Utils
