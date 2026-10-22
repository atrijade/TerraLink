#pragma once

#include "ntn-sim/utils/math_utils.h"
#include <string>
#include <vector>
#include <mutex>

namespace NtnSim::User {

/**
 * @brief 3GPP NTN standardized User Equipment (UE) device categories (TR 38.811 / TR 38.821).
 */
enum class UeClass {
    Handheld,      ///< 3GPP Class 3: Smartphones, Handheld IoT (Omni antenna, 23 dBm)
    Vehicular,     ///< Connected vehicles, high-speed trains (Roof patch, 26 dBm, ~3 dBi)
    Maritime,      ///< Ships, offshore platforms (Stabilized directional dish, 33 dBm, 15-20 dBi)
    Aeronautical,  ///< Aircraft (Fuselage phased array, high speed ~900 km/h, Doppler tolerance)
    FixedVsat      ///< Ground station / Remote enterprise (Parabolic dish 30+ dBi)
};

/**
 * @brief 3GPP 5G NR Radio Resource Control (RRC) state.
 */
enum class RrcState {
    Idle,        ///< Standby / Paging state (no active radio bearer, transmitter off)
    Connected,   ///< Active transmission state (dedicated radio bearer established)
    Inactive     ///< 5G fast-resume low-power state
};

/**
 * @brief Terrestrial mobility movement mode.
 */
enum class MobilityType {
    Stationary,         ///< Fixed coordinates
    LinearGreatCircle,  ///< Constant ground speed and bearing along Earth's spherical curvature
    WaypointTrajectory, ///< Pre-recorded keyframes with linear interpolation
    RandomWaypoint      ///< Roaming within a geographic bounding box
};

/**
 * @brief Terminal RF transceiver characteristics per 3GPP NTN specifications.
 */
struct TerminalRfConfig {
    double txPowerDbm{23.0};          ///< Maximum transmit power in dBm
    double rxAntennaGainDbi{0.0};     ///< Receiver antenna gain in dBi
    double noiseFigureDb{7.0};        ///< Terminal noise figure in dB
    double minElevationDeg{10.0};     ///< Minimum elevation mask angle in degrees
    double maxSpeedMps{0.0};          ///< Maximum operational ground speed in m/s

    static TerminalRfConfig CreateDefault(UeClass ueClass);
};

/**
 * @brief Timestamped waypoint keyframe for trajectory interpolation.
 */
struct TrajectoryWaypoint {
    double timestampSeconds{0.0};
    Utils::GeodeticPosition position;
};

/**
 * @brief Terrestrial mobility configuration.
 */
struct MobilityConfig {
    MobilityType type{MobilityType::Stationary};
    double speedMps{0.0};            ///< Speed in m/s (e.g. 250 m/s for aircraft)
    double bearingDeg{0.0};          ///< Heading azimuth (0 = North, 90 = East, 180 = South, 270 = West)
    std::vector<TrajectoryWaypoint> waypoints;

    // Bounding box for RandomWaypoint mode
    double minLat{-90.0};
    double maxLat{90.0};
    double minLon{-180.0};
    double maxLon{180.0};
};

/**
 * @brief Model representing a User Equipment (UE) terminal in the 5G NTN constellation.
 */
class User {
public:
    User(const std::string& id, const Utils::GeodeticPosition& initialPos,
         UeClass ueClass = UeClass::Handheld);
    ~User();

    // Identification & Device Class
    std::string GetId() const;
    UeClass GetUeClass() const;
    void SetUeClass(UeClass ueClass);

    // RF Profile
    TerminalRfConfig GetRfConfig() const;
    void SetRfConfig(const TerminalRfConfig& config);

    // Geodetic & ECEF Coordinates
    Utils::GeodeticPosition GetPosition() const;
    void SetPosition(const Utils::GeodeticPosition& pos);

    Utils::Position3D GetEcefPosition() const;
    Utils::Position3D GetVelocityEcef() const;
    void SetVelocityEcef(const Utils::Position3D& vel);

    // Mobility
    MobilityConfig GetMobilityConfig() const;
    void SetMobilityConfig(const MobilityConfig& config);
    void UpdatePosition(double currentTimeSeconds);

    // Serving Satellite & Candidates
    std::string GetServingSatelliteId() const;
    void SetServingSatelliteId(const std::string& satId);

    std::vector<std::string> GetCandidateSatelliteIds() const;
    void SetCandidateSatelliteIds(const std::vector<std::string>& candidates);

    // 3GPP RRC State Machine
    RrcState GetRrcState() const;
    void SetRrcState(RrcState state);
    void Connect();
    void Disconnect();
    void SuspendToInactive();
    bool IsConnected() const;

    // Traffic Profile & Inactivity Management
    double GetTrafficRatePps() const;
    void SetTrafficRatePps(double rate);

    double GetInactivityTimeoutSeconds() const;
    void SetInactivityTimeoutSeconds(double timeoutSeconds);
    double GetLastActiveTime() const;
    void TouchActiveTime(double timeSeconds);
    void CheckInactivityTimeout(double currentTimeSeconds);

private:
    std::string m_id;
    UeClass m_ueClass{UeClass::Handheld};
    TerminalRfConfig m_rfConfig;
    MobilityConfig m_mobilityConfig;

    Utils::GeodeticPosition m_position;
    Utils::Position3D m_ecefPosition;
    Utils::Position3D m_velocityEcef{0.0, 0.0, 0.0};
    double m_lastUpdateTimeSeconds{0.0};

    std::string m_servingSatelliteId;
    std::vector<std::string> m_candidateSatelliteIds;

    RrcState m_rrcState{RrcState::Idle};
    double m_trafficRatePps{10.0};
    double m_inactivityTimeoutSeconds{10.0};
    double m_lastActiveTimeSeconds{0.0};

    mutable std::mutex m_mutex;
};

} // namespace NtnSim::User
