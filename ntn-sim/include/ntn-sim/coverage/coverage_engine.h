#pragma once

#include "ntn-sim/coverage/coverage_info.h"
#include <vector>
#include <memory>
#include <string>

namespace NtnSim::Orbit {
class Satellite;
}
namespace NtnSim::User {
class User;
}

namespace NtnSim::Coverage {

/**
 * @brief Configuration parameters for CoverageEngine.
 */
struct CoverageConfig {
    double minElevationDeg{10.0}; ///< ITU-R / 3GPP NTN minimum elevation angle in degrees
};

/**
 * @brief Evaluates spatial visibility, distance, and elevations.
 */
class CoverageEngine {
public:
    CoverageEngine();
    explicit CoverageEngine(const CoverageConfig& config);
    ~CoverageEngine();

    /**
     * @brief Computes coverage metrics between one satellite and one user.
     */
    CoverageInfo CalculateCoverage(const Orbit::Satellite& sat, const User::User& user);

    /**
     * @brief Identifies all satellites that are visible to the user terminal.
     */
    std::vector<std::string> GetVisibleSatellites(const User::User& user, const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites);

    /**
     * @brief Finds and returns the visible satellite with the highest elevation angle for this user.
     * @return Shared pointer to best Satellite, or nullptr if no satellite is visible.
     */
    std::shared_ptr<Orbit::Satellite> GetBestSatellite(const User::User& user, const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites);

    CoverageConfig GetConfig() const;
    void SetConfig(const CoverageConfig& config);

private:
    CoverageConfig m_config;
};

} // namespace NtnSim::Coverage
