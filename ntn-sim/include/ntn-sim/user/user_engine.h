#pragma once

#include "ntn-sim/user/user.h"
#include "ntn-sim/utils/random_generator.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <string>

namespace NtnSim::Core {
class EventScheduler;
}

namespace NtnSim::User {

/**
 * @brief Manages positions, mobility profiles, RRC connection states, and fleet generation
 *        for all terrestrial and airborne User Equipment (UE) terminals.
 */
class UserEngine {
public:
    UserEngine();
    explicit UserEngine(unsigned int randomSeed);
    ~UserEngine();

    // =========================================================================
    // User Registry & O(1) Indexing
    // =========================================================================

    /**
     * @brief Registers a new user terminal in the simulation.
     */
    void AddUser(std::shared_ptr<User> user);

    /**
     * @brief Unregisters a user terminal by ID.
     */
    bool RemoveUser(const std::string& userId);

    /**
     * @brief Retrieves a user terminal by ID (O(1) lookup).
     */
    std::shared_ptr<User> GetUser(const std::string& userId) const;

    /**
     * @brief Retrieves all registered user terminals.
     */
    std::vector<std::shared_ptr<User>> GetAllUsers() const;

    /**
     * @brief Retrieves only users currently in RRC_CONNECTED state.
     */
    std::vector<std::shared_ptr<User>> GetConnectedUsers() const;

    /**
     * @brief Returns total registered user count.
     */
    size_t GetUserCount() const;

    /**
     * @brief Clears all registered users.
     */
    void ClearUsers();

    // =========================================================================
    // Mobility & Position Dynamics
    // =========================================================================

    /**
     * @brief Advances positions and 3D velocity vectors of all users to simTimeSeconds.
     */
    void UpdatePositions(double simTimeSeconds);

    /**
     * @brief Configures default traffic generation rate on all registered users.
     */
    void SetDefaultTrafficProfile(double ppsRate);

    // =========================================================================
    // Automated Fleet & Hotspot Population Generators
    // =========================================================================

    /**
     * @brief Uniformly distributes N users across a geographic bounding box.
     */
    std::vector<std::shared_ptr<User>> GenerateUniformFleet(
        const std::string& prefix, size_t count,
        double minLat, double maxLat, double minLon, double maxLon,
        UeClass ueClass = UeClass::Handheld, double defaultAltMeters = 0.0);

    /**
     * @brief Generates a cluster of N users distributed around a focal center.
     */
    std::vector<std::shared_ptr<User>> GenerateClusteredFleet(
        const std::string& prefix, size_t count,
        const Utils::GeodeticPosition& center, double radiusKm,
        UeClass ueClass = UeClass::Handheld);

    // =========================================================================
    // EventScheduler Integration
    // =========================================================================

    /**
     * @brief Schedules recurring EventType::MoveUser discrete events in the EventScheduler.
     */
    void ScheduleMobilityUpdates(double updateIntervalSeconds, double stopTimeSeconds,
                                Core::EventScheduler& scheduler);

private:
    std::unordered_map<std::string, std::shared_ptr<User>> m_userMap;
    std::vector<std::shared_ptr<User>> m_userList; // Preserves stable insertion order
    Utils::RandomGenerator m_rng;
    double m_lastSimTimeSeconds{0.0};
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::User
