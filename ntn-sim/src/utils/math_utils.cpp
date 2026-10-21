#include "ntn-sim/utils/math_utils.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace NtnSim::Utils {

namespace {
    // WGS-84 ellipsoid constants
    constexpr double WGS84_A  = 6378137.0;          // Semi-major axis (m)
    constexpr double WGS84_E2 = 6.69437999014e-3;   // First eccentricity squared

    inline double DegToRad(double deg) {
        return deg * std::numbers::pi / 180.0;
    }
} // anonymous namespace

double MathUtils::CalculateDistance(const Position3D& pos1, const Position3D& pos2) {
    double dx = pos2.x - pos1.x;
    double dy = pos2.y - pos1.y;
    double dz = pos2.z - pos1.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Position3D MathUtils::GeodeticToEcef(const GeodeticPosition& geo) {
    double lat_rad = DegToRad(geo.latitude);
    double lon_rad = DegToRad(geo.longitude);

    double sin_lat = std::sin(lat_rad);
    double cos_lat = std::cos(lat_rad);
    double sin_lon = std::sin(lon_rad);
    double cos_lon = std::cos(lon_rad);

    // Radius of curvature in the prime vertical
    double N = WGS84_A / std::sqrt(1.0 - WGS84_E2 * sin_lat * sin_lat);

    double x = (N + geo.altitude) * cos_lat * cos_lon;
    double y = (N + geo.altitude) * cos_lat * sin_lon;
    double z = (N * (1.0 - WGS84_E2) + geo.altitude) * sin_lat;

    return Position3D{x, y, z};
}

double MathUtils::CalculateElevationAngle(const Position3D& groundStation, const Position3D& satellite) {
    // Unit vector pointing from Earth centre to the ground station (local zenith)
    double gs_mag = CalculateDistance(Position3D{0.0, 0.0, 0.0}, groundStation);
    if (gs_mag < 1.0) {
        return 90.0;  // Ground station at origin: treat as zenith
    }
    double rhat_x = groundStation.x / gs_mag;
    double rhat_y = groundStation.y / gs_mag;
    double rhat_z = groundStation.z / gs_mag;

    // Vector from ground station to satellite (slant vector)
    double slant_x = satellite.x - groundStation.x;
    double slant_y = satellite.y - groundStation.y;
    double slant_z = satellite.z - groundStation.z;
    double slant_mag = std::sqrt(slant_x * slant_x + slant_y * slant_y + slant_z * slant_z);
    if (slant_mag < 1.0) {
        return 90.0;  // Satellite at same position as ground station
    }

    // Dot product of zenith unit vector with slant unit vector = sin(elevation)
    double sin_elev = (rhat_x * slant_x + rhat_y * slant_y + rhat_z * slant_z) / slant_mag;

    // Clamp to [-1, 1] to guard against floating-point drift
    sin_elev = std::clamp(sin_elev, -1.0, 1.0);

    return std::asin(sin_elev) * 180.0 / std::numbers::pi;
}

} // namespace NtnSim::Utils
