#include <gtest/gtest.h>
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/user/user.h"

TEST(UserEngineTest, AddAndRetrieveUser) {
    NtnSim::User::UserEngine engine;
    
    NtnSim::Utils::GeodeticPosition pos{37.7749, -122.4194, 0.0};
    auto user = std::make_shared<NtnSim::User::User>("user-1", pos);
    user->SetServingSatelliteId("sat-5");
    
    engine.AddUser(user);
    
    auto retrieved = engine.GetUser("user-1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->GetId(), "user-1");
    EXPECT_EQ(retrieved->GetServingSatelliteId(), "sat-5");
    
    EXPECT_EQ(engine.GetUser("non-existent"), nullptr);
    EXPECT_EQ(engine.GetAllUsers().size(), 1);
}
