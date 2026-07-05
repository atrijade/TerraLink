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
 * @brief Evaluates spatial visibility, distance, and elevations.
 */
class CoverageEngine {
public:
    CoverageEngine();
    ~CoverageEngine();

    /**
     * @brief Computes coverage metrics between one satellite and one user.
     */
    CoverageInfo CalculateCoverage(const Orbit::Satellite& sat, const User::User& user);

    /**
     * @brief Identifies all satellites that are visible to the user terminal.
     */
    std::vector<std::string> GetVisibleSatellites(const User::User& user, const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites);
};

} // namespace NtnSim::Coverage
