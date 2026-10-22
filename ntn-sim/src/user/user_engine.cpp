#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/core/event.h"
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

UserEngine::UserEngine() {}

UserEngine::UserEngine(unsigned int randomSeed)
    : m_rng(randomSeed) {}

UserEngine::~UserEngine() {}

void UserEngine::AddUser(std::shared_ptr<User> user) {
    if (!user) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_userMap.find(user->GetId());
    if (it == m_userMap.end()) {
        m_userMap[user->GetId()] = user;
        m_userList.push_back(user);
    } else {
        it->second = user;
        for (auto& u : m_userList) {
            if (u->GetId() == user->GetId()) {
                u = user;
                break;
            }
        }
    }
}

bool UserEngine::RemoveUser(const std::string& userId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_userMap.find(userId);
    if (it != m_userMap.end()) {
        m_userMap.erase(it);
        auto listIt = std::remove_if(m_userList.begin(), m_userList.end(),
                                     [&userId](const auto& u) { return u->GetId() == userId; });
        m_userList.erase(listIt, m_userList.end());
        return true;
    }
    return false;
}

std::shared_ptr<User> UserEngine::GetUser(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_userMap.find(userId);
    if (it != m_userMap.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<User>> UserEngine::GetAllUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_userList;
}

std::vector<std::shared_ptr<User>> UserEngine::GetConnectedUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<User>> connected;
    for (const auto& u : m_userList) {
        if (u->IsConnected()) {
            connected.push_back(u);
        }
    }
    return connected;
}

size_t UserEngine::GetUserCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_userMap.size();
}

void UserEngine::ClearUsers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_userMap.clear();
    m_userList.clear();
}

void UserEngine::UpdatePositions(double simTimeSeconds) {
    std::vector<std::shared_ptr<User>> usersCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastSimTimeSeconds = simTimeSeconds;
        usersCopy = m_userList;
    }

    for (auto& u : usersCopy) {
        u->UpdatePosition(simTimeSeconds);
        u->CheckInactivityTimeout(simTimeSeconds);
    }
}

void UserEngine::SetDefaultTrafficProfile(double ppsRate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& u : m_userList) {
        u->SetTrafficRatePps(ppsRate);
    }
}

std::vector<std::shared_ptr<User>> UserEngine::GenerateUniformFleet(
    const std::string& prefix, size_t count,
    double minLat, double maxLat, double minLon, double maxLon,
    UeClass ueClass, double defaultAltMeters) {

    std::vector<std::shared_ptr<User>> createdUsers;
    createdUsers.reserve(count);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < count; ++i) {
        double lat = m_rng.GetUniformDouble(minLat, maxLat);
        double lon = m_rng.GetUniformDouble(minLon, maxLon);
        std::string id = prefix + "-" + std::to_string(i + 1);

        Utils::GeodeticPosition pos{lat, lon, defaultAltMeters};
        auto user = std::make_shared<User>(id, pos, ueClass);

        m_userMap[id] = user;
        m_userList.push_back(user);
        createdUsers.push_back(user);
    }

    return createdUsers;
}

std::vector<std::shared_ptr<User>> UserEngine::GenerateClusteredFleet(
    const std::string& prefix, size_t count,
    const Utils::GeodeticPosition& center, double radiusKm,
    UeClass ueClass) {

    std::vector<std::shared_ptr<User>> createdUsers;
    createdUsers.reserve(count);

    double phi1 = DegToRad(center.latitude);
    double lambda1 = DegToRad(center.longitude);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < count; ++i) {
        // Uniform distribution over a disk: r = R * sqrt(u)
        double u = m_rng.GetUniformDouble(0.0001, 1.0);
        double distMeters = (radiusKm * 1000.0) * std::sqrt(u);
        double bearingDeg = m_rng.GetUniformDouble(0.0, 360.0);

        double delta = distMeters / WGS84_A;
        double theta = DegToRad(bearingDeg);

        double sinPhi1 = std::sin(phi1);
        double cosPhi1 = std::cos(phi1);
        double sinDelta = std::sin(delta);
        double cosDelta = std::cos(delta);

        double sinPhi2 = sinPhi1 * cosDelta + cosPhi1 * sinDelta * std::cos(theta);
        double phi2 = std::asin(std::clamp(sinPhi2, -1.0, 1.0));

        double y = std::sin(theta) * sinDelta * cosPhi1;
        double x = cosDelta - sinPhi1 * sinPhi2;
        double lambda2 = lambda1 + std::atan2(y, x);

        double newLat = std::clamp(RadToDeg(phi2), -90.0, 90.0);
        double newLon = RadToDeg(lambda2);
        while (newLon > 180.0) newLon -= 360.0;
        while (newLon < -180.0) newLon += 360.0;

        std::string id = prefix + "-" + std::to_string(i + 1);
        Utils::GeodeticPosition pos{newLat, newLon, center.altitude};
        auto user = std::make_shared<User>(id, pos, ueClass);

        m_userMap[id] = user;
        m_userList.push_back(user);
        createdUsers.push_back(user);
    }

    return createdUsers;
}

void UserEngine::ScheduleMobilityUpdates(double updateIntervalSeconds, double stopTimeSeconds,
                                        Core::EventScheduler& scheduler) {
    if (updateIntervalSeconds <= 0.0) {
        return;
    }

    auto stepUpdate = std::make_shared<std::function<void(double)>>();
    *stepUpdate = [this, updateIntervalSeconds, stopTimeSeconds, &scheduler, stepUpdate](double currentTime) {
        if (currentTime > stopTimeSeconds) {
            return;
        }

        UpdatePositions(currentTime);

        double nextTime = currentTime + updateIntervalSeconds;
        if (nextTime <= stopTimeSeconds) {
            scheduler.ScheduleEvent(nextTime, 8, Core::EventType::MoveUser, [stepUpdate, nextTime]() {
                (*stepUpdate)(nextTime);
            });
        }
    };

    // Schedule initial movement update at t = updateIntervalSeconds
    scheduler.ScheduleEvent(updateIntervalSeconds, 8, Core::EventType::MoveUser, [stepUpdate, updateIntervalSeconds]() {
        (*stepUpdate)(updateIntervalSeconds);
    });
}

} // namespace NtnSim::User
