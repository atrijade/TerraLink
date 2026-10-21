#include "ntn-sim/coverage/coverage_engine.h"
#include "ntn-sim/orbit/satellite.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/utils/math_utils.h"
#include <cmath>
#include <algorithm>

namespace NtnSim::Coverage {

namespace {
    constexpr double MIN_ELEVATION_DEG = 10.0;  // 3GPP NTN minimum elevation threshold
} // anonymous namespace

CoverageEngine::CoverageEngine() {}

CoverageEngine::~CoverageEngine() {}

CoverageInfo CoverageEngine::CalculateCoverage(const Orbit::Satellite& sat, const User::User& user) {
    CoverageInfo info;

    // Step 1: Convert user geodetic (lat, lon, alt) -> ECEF (X, Y, Z) in metres
    Utils::Position3D userEcef = Utils::MathUtils::GeodeticToEcef(user.GetPosition());

    // Step 2: Get satellite ECEF position (in metres — orbit_state stores metres internally)
    Utils::Position3D satEcef = sat.GetOrbitState().GetEcefPosition();

    // Step 3: Compute slant range distance (m)
    double distanceMeters = Utils::MathUtils::CalculateDistance(userEcef, satEcef);
    info.SetDistanceMeters(distanceMeters);

    // Step 4: Compute elevation angle (deg)
    double elevationDeg = Utils::MathUtils::CalculateElevationAngle(userEcef, satEcef);
    info.SetElevationAngleDeg(elevationDeg);

    // Step 5: Visibility — satellite must be above minimum elevation threshold
    bool visible = (elevationDeg >= MIN_ELEVATION_DEG);
    info.SetVisible(visible);

    // Step 6: Compute LoS velocity component for Doppler shift (km/s)
    //   v_los = v_sat_ecef (km/s) · unit_vector(sat - user)
    if (visible && distanceMeters > 0.0) {
        Utils::Position3D satVelEcef = sat.GetOrbitState().GetEcefVelocity(); // km/s

        double slant_x = satEcef.x - userEcef.x;
        double slant_y = satEcef.y - userEcef.y;
        double slant_z = satEcef.z - userEcef.z;
        // Unit vector from user toward satellite
        double uhat_x = slant_x / distanceMeters;
        double uhat_y = slant_y / distanceMeters;
        double uhat_z = slant_z / distanceMeters;

        // Dot product: v_los > 0 means satellite approaching user (frequency upshift)
        // sat velocity is in km/s, uhat is dimensionless
        double losVelocityKms = satVelEcef.x * uhat_x
                              + satVelEcef.y * uhat_y
                              + satVelEcef.z * uhat_z;
        info.SetLosVelocityKms(-losVelocityKms); // Negate: positive = approaching
    } else {
        info.SetLosVelocityKms(0.0);
    }

    return info;
}

std::vector<std::string> CoverageEngine::GetVisibleSatellites(
    const User::User& user,
    const std::vector<std::shared_ptr<Orbit::Satellite>>& satellites)
{
    std::vector<std::string> visibleIds;
    for (const auto& sat : satellites) {
        CoverageInfo info = CalculateCoverage(*sat, user);
        if (info.IsVisible()) {
            visibleIds.push_back(sat->GetId());
        }
    }
    return visibleIds;
}

} // namespace NtnSim::Coverage
