#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"

namespace NtnSim::Coverage {

CoverageEngine::CoverageEngine() {}

CoverageEngine::~CoverageEngine() {}

CoverageInfo CoverageEngine::CalculateCoverage(const Orbit::Satellite& sat, const User::User& user) {
    (void)sat;
    (void)user;
    // Skeleton implementation
    CoverageInfo info;
    info.SetVisible(true);
    info.SetDistanceMeters(1000000.0);
    info.SetElevationAngleDeg(45.0);
    return info;
}

std::vector<std::string> CoverageEngine::GetVisibleSatellites(const User::User& user, const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) {
    (void)user;
    std::vector<std::string> visibleIds;
    for (const auto& sat : satellites) {
        visibleIds.push_back(sat->GetId());
    }
    return visibleIds;
}

} // namespace NtnSim::Coverage
