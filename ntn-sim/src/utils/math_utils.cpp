#include "ntn-sim/utils/math_utils.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace NtnSim::Utils {

namespace {
    // WGS-84 ellipsoid constants
    constexpr double WGS84_A  = 6378137.0;          // Semi-major axis (m)
    constexpr double WGS84_E2 = 6.69437999014e-3;   // First eccentricity squared
    constexpr double WGS84_F  = 1.0 / 298.257223563;// Flattening
    constexpr double WGS84_B  = WGS84_A * (1.0 - WGS84_F); // Semi-minor axis (m)
    constexpr double WGS84_EP2 = (WGS84_A * WGS84_A - WGS84_B * WGS84_B) / (WGS84_B * WGS84_B);

    inline double DegToRad(double deg) {
        return deg * std::numbers::pi / 180.0;
    }

    inline double RadToDeg(double rad) {
        return rad * 180.0 / std::numbers::pi;
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

GeodeticPosition MathUtils::EcefToGeodetic(const Position3D& ecef) {
    double p = std::sqrt(ecef.x * ecef.x + ecef.y * ecef.y);
    if (p < 1e-6) {
        // Polar singularities
        double lat = (ecef.z >= 0.0) ? 90.0 : -90.0;
        double alt = std::abs(ecef.z) - WGS84_B;
        return GeodeticPosition{lat, 0.0, alt};
    }

    // Bowring's closed-form algorithm for WGS-84
    double theta = std::atan2(ecef.z * WGS84_A, p * WGS84_B);
    double sinTheta = std::sin(theta);
    double cosTheta = std::cos(theta);

    double latRad = std::atan2(ecef.z + WGS84_EP2 * WGS84_B * sinTheta * sinTheta * sinTheta,
                               p - WGS84_E2 * WGS84_A * cosTheta * cosTheta * cosTheta);
    double lonRad = std::atan2(ecef.y, ecef.x);

    double sinLat = std::sin(latRad);
    double N = WGS84_A / std::sqrt(1.0 - WGS84_E2 * sinLat * sinLat);
    double alt = (p / std::cos(latRad)) - N;

    return GeodeticPosition{RadToDeg(latRad), RadToDeg(lonRad), alt};
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

    // Vector from ground station to satellite
    double slant_x = satellite.x - groundStation.x;
    double slant_y = satellite.y - groundStation.y;
    double slant_z = satellite.z - groundStation.z;
    double slant_range = std::sqrt(slant_x * slant_x + slant_y * slant_y + slant_z * slant_z);

    if (slant_range < 1e-3) {
        return 90.0;
    }

    // Dot product with local vertical gives sin(elevation)
    double sin_elevation = (slant_x * rhat_x + slant_y * rhat_y + slant_z * rhat_z) / slant_range;

    // Clamp to valid range [-1.0, 1.0] for safety against floating point inaccuracies
    sin_elevation = std::clamp(sin_elevation, -1.0, 1.0);

    return std::asin(sin_elevation) * 180.0 / std::numbers::pi;
}

} // namespace NtnSim::Utils
