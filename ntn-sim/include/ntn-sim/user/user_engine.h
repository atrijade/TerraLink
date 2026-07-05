#pragma once

#include "ntn-sim/user/user.h"
#include <vector>
#include <memory>
#include <mutex>
#include <string>

namespace NtnSim::User {

/**
 * @brief Manages positions, traffic states, and serving satellites of all mobile terminals.
 */
class UserEngine {
public:
    UserEngine();
    ~UserEngine();

    /**
     * @brief Computes dynamic coordinates for all terminals based on mobility profiles.
     * @param simTimeSeconds Current simulation timestamp.
     */
    void UpdatePositions(double simTimeSeconds);

    /**
     * @brief Registers a user terminal.
     */
    void AddUser(std::shared_ptr<User> user);

    /**
     * @brief Gets user details by ID.
     */
    std::shared_ptr<User> GetUser(const std::string& userId) const;

    /**
     * @brief Gets all registered user terminals.
     */
    std::vector<std::shared_ptr<User>> GetAllUsers() const;

    /**
     * @brief Configures traffic profiles (packets rates etc) on all users.
     */
    void SetDefaultTrafficProfile(double ppsRate);

private:
    std::vector<std::shared_ptr<User>> m_users;
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::User
