#include "ntn-sim/user/user.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace NtnSim::User {

namespace {
    constexpr double WGS84_A = 6378137.0; // Earth equatorial radius in meters

    inline double DegToRad(double deg) {
        return deg * std::numbers::pi / 180.0;
    }

    inline double RadToDeg(double rad) {
        return rad * 180.0 / std::numbers::pi;
    }
} // anonymous namespace

TerminalRfConfig TerminalRfConfig::CreateDefault(UeClass ueClass) {
    TerminalRfConfig cfg;
    switch (ueClass) {
        case UeClass::Handheld:
            cfg.txPowerDbm = 23.0;
            cfg.rxAntennaGainDbi = 0.0;
            cfg.noiseFigureDb = 7.0;
            cfg.minElevationDeg = 10.0;
            cfg.maxSpeedMps = 15.0; // 54 km/h pedestrian/urban
            break;
        case UeClass::Vehicular:
            cfg.txPowerDbm = 26.0;
            cfg.rxAntennaGainDbi = 3.0;
            cfg.noiseFigureDb = 7.0;
            cfg.minElevationDeg = 10.0;
            cfg.maxSpeedMps = 45.0; // 162 km/h highway/train
            break;
        case UeClass::Maritime:
            cfg.txPowerDbm = 33.0;
            cfg.rxAntennaGainDbi = 18.0;
            cfg.noiseFigureDb = 5.0;
            cfg.minElevationDeg = 5.0;
            cfg.maxSpeedMps = 25.0; // 90 km/h fast vessel
            break;
        case UeClass::Aeronautical:
            cfg.txPowerDbm = 33.0;
            cfg.rxAntennaGainDbi = 15.0;
            cfg.noiseFigureDb = 5.0;
            cfg.minElevationDeg = 5.0;
            cfg.maxSpeedMps = 280.0; // 1008 km/h jet aircraft
            break;
        case UeClass::FixedVsat:
            cfg.txPowerDbm = 33.0;
            cfg.rxAntennaGainDbi = 35.0;
            cfg.noiseFigureDb = 4.0;
            cfg.minElevationDeg = 5.0;
            cfg.maxSpeedMps = 0.0;
            break;
    }
    return cfg;
}

User::User(const std::string& id, const Utils::GeodeticPosition& initialPos, UeClass ueClass)
    : m_id(id),
      m_ueClass(ueClass),
      m_rfConfig(TerminalRfConfig::CreateDefault(ueClass)),
      m_position(initialPos),
      m_ecefPosition(Utils::MathUtils::GeodeticToEcef(initialPos)) {}

User::~User() {}

std::string User::GetId() const {
    return m_id;
}

UeClass User::GetUeClass() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ueClass;
}

void User::SetUeClass(UeClass ueClass) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ueClass = ueClass;
    m_rfConfig = TerminalRfConfig::CreateDefault(ueClass);
}

TerminalRfConfig User::GetRfConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rfConfig;
}

void User::SetRfConfig(const TerminalRfConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rfConfig = config;
}

Utils::GeodeticPosition User::GetPosition() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_position;
}

void User::SetPosition(const Utils::GeodeticPosition& pos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_position = pos;
    m_ecefPosition = Utils::MathUtils::GeodeticToEcef(pos);
}

Utils::Position3D User::GetEcefPosition() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ecefPosition;
}

Utils::Position3D User::GetVelocityEcef() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_velocityEcef;
}

void User::SetVelocityEcef(const Utils::Position3D& vel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_velocityEcef = vel;
}

MobilityConfig User::GetMobilityConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mobilityConfig;
}

void User::SetMobilityConfig(const MobilityConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mobilityConfig = config;
}

void User::UpdatePosition(double currentTimeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (currentTimeSeconds <= m_lastUpdateTimeSeconds) {
        return;
    }

    double dt = currentTimeSeconds - m_lastUpdateTimeSeconds;
    m_lastUpdateTimeSeconds = currentTimeSeconds;

    if (m_mobilityConfig.type == MobilityType::Stationary) {
        m_velocityEcef = Utils::Position3D{0.0, 0.0, 0.0};
        return;
    }

    Utils::Position3D oldEcef = m_ecefPosition;

    if (m_mobilityConfig.type == MobilityType::LinearGreatCircle) {
        double distMeters = m_mobilityConfig.speedMps * dt;
        double delta = distMeters / WGS84_A; // Angular distance in radians
        double theta = DegToRad(m_mobilityConfig.bearingDeg);

        double phi1 = DegToRad(m_position.latitude);
        double lambda1 = DegToRad(m_position.longitude);

        double sinPhi1 = std::sin(phi1);
        double cosPhi1 = std::cos(phi1);
        double sinDelta = std::sin(delta);
        double cosDelta = std::cos(delta);

        // Great-Circle destination equations
        double sinPhi2 = sinPhi1 * cosDelta + cosPhi1 * sinDelta * std::cos(theta);
        double phi2 = std::asin(std::clamp(sinPhi2, -1.0, 1.0));

        double y = std::sin(theta) * sinDelta * cosPhi1;
        double x = cosDelta - sinPhi1 * sinPhi2;
        double lambda2 = lambda1 + std::atan2(y, x);

        double newLat = RadToDeg(phi2);
        double newLon = RadToDeg(lambda2);

        // Clamp latitude to [-90, 90]
        newLat = std::clamp(newLat, -90.0, 90.0);

        // Normalize longitude to [-180, 180]
        while (newLon > 180.0) newLon -= 360.0;
        while (newLon < -180.0) newLon += 360.0;

        m_position.latitude = newLat;
        m_position.longitude = newLon;
        m_ecefPosition = Utils::MathUtils::GeodeticToEcef(m_position);
    } else if (m_mobilityConfig.type == MobilityType::WaypointTrajectory) {
        const auto& wps = m_mobilityConfig.waypoints;
        if (!wps.empty()) {
            if (currentTimeSeconds <= wps.front().timestampSeconds) {
                m_position = wps.front().position;
            } else if (currentTimeSeconds >= wps.back().timestampSeconds) {
                m_position = wps.back().position;
            } else {
                for (size_t i = 0; i < wps.size() - 1; ++i) {
                    if (currentTimeSeconds >= wps[i].timestampSeconds &&
                        currentTimeSeconds <= wps[i + 1].timestampSeconds) {
                        double span = wps[i + 1].timestampSeconds - wps[i].timestampSeconds;
                        double alpha = (span > 0.0) ? (currentTimeSeconds - wps[i].timestampSeconds) / span : 0.0;
                        m_position.latitude = wps[i].position.latitude + alpha * (wps[i + 1].position.latitude - wps[i].position.latitude);
                        m_position.longitude = wps[i].position.longitude + alpha * (wps[i + 1].position.longitude - wps[i].position.longitude);
                        m_position.altitude = wps[i].position.altitude + alpha * (wps[i + 1].position.altitude - wps[i].position.altitude);
                        break;
                    }
                }
            }
            m_ecefPosition = Utils::MathUtils::GeodeticToEcef(m_position);
        }
    }

    if (dt > 0.0) {
        m_velocityEcef = Utils::Position3D{
            (m_ecefPosition.x - oldEcef.x) / dt,
            (m_ecefPosition.y - oldEcef.y) / dt,
            (m_ecefPosition.z - oldEcef.z) / dt
        };
    }
}

std::string User::GetServingSatelliteId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_servingSatelliteId;
}

void User::SetServingSatelliteId(const std::string& satId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_servingSatelliteId = satId;
}

std::vector<std::string> User::GetCandidateSatelliteIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_candidateSatelliteIds;
}

void User::SetCandidateSatelliteIds(const std::vector<std::string>& candidates) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_candidateSatelliteIds = candidates;
}

RrcState User::GetRrcState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rrcState;
}

void User::SetRrcState(RrcState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rrcState = state;
}

void User::Connect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rrcState = RrcState::Connected;
}

void User::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rrcState = RrcState::Idle;
    m_servingSatelliteId.clear();
}

void User::SuspendToInactive() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rrcState = RrcState::Inactive;
}

bool User::IsConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rrcState == RrcState::Connected;
}

double User::GetTrafficRatePps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trafficRatePps;
}

void User::SetTrafficRatePps(double rate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trafficRatePps = rate;
}

double User::GetInactivityTimeoutSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_inactivityTimeoutSeconds;
}

void User::SetInactivityTimeoutSeconds(double timeoutSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_inactivityTimeoutSeconds = timeoutSeconds;
}

double User::GetLastActiveTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastActiveTimeSeconds;
}

void User::TouchActiveTime(double timeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastActiveTimeSeconds = timeSeconds;
}

void User::CheckInactivityTimeout(double currentTimeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_rrcState == RrcState::Connected && m_inactivityTimeoutSeconds > 0.0) {
        if ((currentTimeSeconds - m_lastActiveTimeSeconds) >= m_inactivityTimeoutSeconds) {
            m_rrcState = RrcState::Idle;
        }
    }
}

} // namespace NtnSim::User
