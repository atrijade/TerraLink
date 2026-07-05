#include "ntn-sim/user/user_engine.h"
#include <algorithm>

namespace NtnSim::User {

UserEngine::UserEngine() {}

UserEngine::~UserEngine() {}

void UserEngine::UpdatePositions(double simTimeSeconds) {
    (void)simTimeSeconds;
    // Skeleton implementation
}

void UserEngine::AddUser(std::shared_ptr<User> user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_users.push_back(user);
}

std::shared_ptr<User> UserEngine::GetUser(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_users.begin(), m_users.end(), [&userId](const auto& user) {
        return user->GetId() == userId;
    });
    if (it != m_users.end()) {
        return *it;
    }
    return nullptr;
}

std::vector<std::shared_ptr<User>> UserEngine::GetAllUsers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_users;
}

void UserEngine::SetDefaultTrafficProfile(double ppsRate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& u : m_users) {
        u->SetTrafficRatePps(ppsRate);
    }
}

} // namespace NtnSim::User
