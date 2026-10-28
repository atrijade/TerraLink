#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/utils/math_utils.h"

namespace NtnSim::Coverage {

CoverageEngine::CoverageEngine() {}

CoverageEngine::CoverageEngine(const CoverageConfig& config)
    : m_config(config) {}

CoverageEngine::~CoverageEngine() {}

CoverageInfo CoverageEngine::CalculateCoverage(const Orbit::Satellite& sat, const Utils::GeodeticPosition& groundPos) const {
    CoverageInfo info;

    // Step 1: Convert ground geodetic (lat, lon, alt) -> ECEF (X, Y, Z) in metres
    Utils::Position3D groundEcef = Utils::MathUtils::GeodeticToEcef(groundPos);

    // Step 2: Get satellite ECEF position (metres)
    Utils::Position3D satEcef = sat.GetOrbitState().GetEcefPosition();

    // Step 3: Compute slant range distance (m)
    double distanceMeters = Utils::MathUtils::CalculateDistance(groundEcef, satEcef);
    info.SetDistanceMeters(distanceMeters);

    // Step 4: Compute elevation angle (deg)
    double elevationDeg = Utils::MathUtils::CalculateElevationAngle(groundEcef, satEcef);
    info.SetElevationAngleDeg(elevationDeg);

    // Step 5: Visibility — satellite must be above minimum elevation threshold
    bool visible = (elevationDeg >= m_config.minElevationDeg);
    info.SetVisible(visible);

    // Step 6: Compute LoS velocity component for Doppler shift (km/s)
    if (visible && distanceMeters > 0.0) {
        Utils::Position3D satVelEcef = sat.GetOrbitState().GetEcefVelocity(); // km/s

        double slant_x = satEcef.x - groundEcef.x;
        double slant_y = satEcef.y - groundEcef.y;
        double slant_z = satEcef.z - groundEcef.z;

        // Unit vector from ground station toward satellite
        double uhat_x = slant_x / distanceMeters;
        double uhat_y = slant_y / distanceMeters;
        double uhat_z = slant_z / distanceMeters;

        // Dot product: positive = satellite approaching user
        double losVelocityKms = satVelEcef.x * uhat_x
                              + satVelEcef.y * uhat_y
                              + satVelEcef.z * uhat_z;
        info.SetLosVelocityKms(-losVelocityKms);
    } else {
        info.SetLosVelocityKms(0.0);
    }

    return info;
}

CoverageInfo CoverageEngine::CalculateCoverage(const Orbit::Satellite& sat, const User::User& user) const {
    return CalculateCoverage(sat, user.GetPosition());
}

CoverageInfo CoverageEngine::CalculateCoverage(const Orbit::Satellite& sat, const Gateway::Gateway& gateway) const {
    return CalculateCoverage(sat, gateway.GetPosition());
}

std::vector<std::string> CoverageEngine::GetVisibleSatellites(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) const
{
    std::vector<std::string> visibleIds;
    for (const auto& sat : satellites) {
        if (!sat) continue;
        CoverageInfo info = CalculateCoverage(*sat, user);
        if (info.IsVisible()) {
            visibleIds.push_back(sat->GetId());
        }
    }
    return visibleIds;
}

std::vector<std::string> CoverageEngine::GetVisibleSatellites(
    const Gateway::Gateway& gateway,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) const
{
    std::vector<std::string> visibleIds;
    for (const auto& sat : satellites) {
        if (!sat) continue;
        CoverageInfo info = CalculateCoverage(*sat, gateway);
        if (info.IsVisible()) {
            visibleIds.push_back(sat->GetId());
        }
    }
    return visibleIds;
}

std::shared_ptr<Orbit::Satellite> CoverageEngine::GetBestSatellite(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) const
{
    std::shared_ptr<Orbit::Satellite> bestSat = nullptr;
    double maxElevation = -90.0;

    for (const auto& sat : satellites) {
        if (!sat) continue;
        CoverageInfo info = CalculateCoverage(*sat, user);
        if (info.IsVisible() && info.GetElevationAngleDeg() > maxElevation) {
            maxElevation = info.GetElevationAngleDeg();
            bestSat = sat;
        }
    }
    return bestSat;
}

std::shared_ptr<Orbit::Satellite> CoverageEngine::GetBestSatellite(
    const Gateway::Gateway& gateway,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites) const
{
    std::shared_ptr<Orbit::Satellite> bestSat = nullptr;
    double maxElevation = -90.0;

    for (const auto& sat : satellites) {
        if (!sat) continue;
        CoverageInfo info = CalculateCoverage(*sat, gateway);
        if (info.IsVisible() && info.GetElevationAngleDeg() > maxElevation) {
            maxElevation = info.GetElevationAngleDeg();
            bestSat = sat;
        }
    }
    return bestSat;
}

CoverageConfig CoverageEngine::GetConfig() const {
    return m_config;
}

void CoverageEngine::SetConfig(const CoverageConfig& config) {
    m_config = config;
}

} // namespace NtnSim::Coverage
