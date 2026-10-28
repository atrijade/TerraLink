#pragma once

namespace NtnSim::Utils {

/**
 * @brief Represents a point in 3D ECEF (Earth-Centered, Earth-Fixed) space.
 */
struct Position3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

/**
 * @brief Represents geodetic coordinates (Latitude, Longitude, Altitude).
 */
struct GeodeticPosition {
    double latitude{0.0};   ///< Latitude in degrees
    double longitude{0.0};  ///< Longitude in degrees
    double altitude{0.0};   ///< Altitude in meters above ellipsoid
};

/**
 * @brief Mathematical utility functions for NTN calculations.
 */
class MathUtils {
public:
    MathUtils() = delete;
    ~MathUtils() = delete;

    /**
     * @brief Computes the Euclidean distance between two 3D points.
     * @param pos1 First position.
     * @param pos2 Second position.
     * @return Distance in meters.
     */
    static double CalculateDistance(const Position3D& pos1, const Position3D& pos2);

    /**
     * @brief Converts geodetic coordinates (LLA) to ECEF coordinates.
     * @param geo Geodetic position.
     * @return 3D position in ECEF frame.
     */
    static Position3D GeodeticToEcef(const GeodeticPosition& geo);

    /**
     * @brief Converts ECEF coordinates (metres) to Geodetic coordinates (WGS-84).
     * @param ecef 3D position in ECEF frame.
     * @return Geodetic position (Latitude deg, Longitude deg, Altitude m).
     */
    static GeodeticPosition EcefToGeodetic(const Position3D& ecef);

    /**
     * @brief Calculates elevation angle from a ground station to a satellite.
     * @param groundStation ECEF position of ground station.
     * @param satellite ECEF position of satellite.
     * @return Elevation angle in degrees.
     */
    static double CalculateElevationAngle(const Position3D& groundStation, const Position3D& satellite);
};

} // namespace NtnSim::Utils
