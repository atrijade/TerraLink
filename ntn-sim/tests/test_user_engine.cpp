#include <gtest/gtest.h>
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/user/user.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/utils/math_utils.h"
#include <cmath>
#include <set>

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
    EXPECT_EQ(engine.GetAllUsers().size(), 1u);
    EXPECT_EQ(engine.GetUserCount(), 1u);

    EXPECT_TRUE(engine.RemoveUser("user-1"));
    EXPECT_EQ(engine.GetUser("user-1"), nullptr);
    EXPECT_EQ(engine.GetUserCount(), 0u);
}

TEST(UserEngineTest, UeClassAndRfProfiles) {
    NtnSim::Utils::GeodeticPosition pos{0.0, 0.0, 0.0};

    // 1. Handheld
    NtnSim::User::User handheld("ue-phone", pos, NtnSim::User::UeClass::Handheld);
    EXPECT_EQ(handheld.GetUeClass(), NtnSim::User::UeClass::Handheld);
    EXPECT_DOUBLE_EQ(handheld.GetRfConfig().txPowerDbm, 23.0);
    EXPECT_DOUBLE_EQ(handheld.GetRfConfig().rxAntennaGainDbi, 0.0);
    EXPECT_DOUBLE_EQ(handheld.GetRfConfig().minElevationDeg, 10.0);

    // 2. Aeronautical (Jet aircraft)
    NtnSim::User::User aero("ue-plane", pos, NtnSim::User::UeClass::Aeronautical);
    EXPECT_EQ(aero.GetUeClass(), NtnSim::User::UeClass::Aeronautical);
    EXPECT_DOUBLE_EQ(aero.GetRfConfig().txPowerDbm, 33.0);
    EXPECT_DOUBLE_EQ(aero.GetRfConfig().rxAntennaGainDbi, 15.0);
    EXPECT_DOUBLE_EQ(aero.GetRfConfig().maxSpeedMps, 280.0);

    // 3. Fixed VSAT
    NtnSim::User::User vsat("ue-vsat", pos, NtnSim::User::UeClass::FixedVsat);
    EXPECT_DOUBLE_EQ(vsat.GetRfConfig().rxAntennaGainDbi, 35.0);
    EXPECT_DOUBLE_EQ(vsat.GetRfConfig().maxSpeedMps, 0.0);
}

TEST(UserEngineTest, RrcStateMachineTransitions) {
    NtnSim::Utils::GeodeticPosition pos{40.7128, -74.0060, 10.0};
    auto user = std::make_shared<NtnSim::User::User>("ue-rrc", pos);

    // Initial state must be RRC_IDLE
    EXPECT_EQ(user->GetRrcState(), NtnSim::User::RrcState::Idle);
    EXPECT_FALSE(user->IsConnected());

    // Connect -> RRC_CONNECTED
    user->Connect();
    user->SetServingSatelliteId("sat-10");
    EXPECT_EQ(user->GetRrcState(), NtnSim::User::RrcState::Connected);
    EXPECT_TRUE(user->IsConnected());
    EXPECT_EQ(user->GetServingSatelliteId(), "sat-10");

    // Suspend to INACTIVE
    user->SuspendToInactive();
    EXPECT_EQ(user->GetRrcState(), NtnSim::User::RrcState::Inactive);
    EXPECT_FALSE(user->IsConnected());

    // Re-connect
    user->Connect();
    EXPECT_TRUE(user->IsConnected());

    // Disconnect -> IDLE (clears serving sat)
    user->Disconnect();
    EXPECT_EQ(user->GetRrcState(), NtnSim::User::RrcState::Idle);
    EXPECT_FALSE(user->IsConnected());
    EXPECT_TRUE(user->GetServingSatelliteId().empty());
}

TEST(UserEngineTest, RrcInactivityTimeout) {
    NtnSim::Utils::GeodeticPosition pos{0.0, 0.0, 0.0};
    auto user = std::make_shared<NtnSim::User::User>("ue-timer", pos);

    user->Connect();
    user->SetInactivityTimeoutSeconds(5.0);
    user->TouchActiveTime(10.0); // Last active at t = 10.0s

    // At t = 12.0s, elapsed = 2.0s <= 5.0s -> still CONNECTED
    user->CheckInactivityTimeout(12.0);
    EXPECT_TRUE(user->IsConnected());

    // At t = 15.1s, elapsed = 5.1s > 5.0s -> transitions to IDLE
    user->CheckInactivityTimeout(15.1);
    EXPECT_FALSE(user->IsConnected());
    EXPECT_EQ(user->GetRrcState(), NtnSim::User::RrcState::Idle);
}

TEST(UserEngineTest, GreatCircleLinearMotionDisplacement) {
    // Start at Equator (0 deg N, 0 deg E)
    NtnSim::Utils::GeodeticPosition startPos{0.0, 0.0, 0.0};
    auto user = std::make_shared<NtnSim::User::User>("aircraft-1", startPos, NtnSim::User::UeClass::Aeronautical);

    // Heading due North (bearing = 0 deg) at speed = 250 m/s (~900 km/h)
    NtnSim::User::MobilityConfig mob;
    mob.type = NtnSim::User::MobilityType::LinearGreatCircle;
    mob.speedMps = 250.0;
    mob.bearingDeg = 0.0; // Due North
    user->SetMobilityConfig(mob);

    // Update position after 3600 seconds (1 hour):
    // Total distance = 250 * 3600 = 900,000 meters = 900 km
    // Angular displacement = 900,000 / 6,378,137 rad ~= 0.141107 rad ~= 8.0848 deg North
    user->UpdatePosition(3600.0);

    auto newPos = user->GetPosition();
    EXPECT_NEAR(newPos.latitude, 8.0848, 0.05); // Displaced ~8.08 degrees North
    EXPECT_NEAR(newPos.longitude, 0.0, 1e-4);   // Heading North keeps longitude at 0

    // Velocity in ECEF frame should have magnitude ~= 250 m/s
    auto vel = user->GetVelocityEcef();
    double speedMagnitude = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
    EXPECT_NEAR(speedMagnitude, 250.0, 3.0); // Within 1% over 1-hour 900km chord across oblate Earth

    // Over standard 1-second simulation step, ECEF speed on WGS-84 ellipsoid is within 1% of ground speed
    auto uStep = std::make_shared<NtnSim::User::User>("aircraft-step", startPos, NtnSim::User::UeClass::Aeronautical);
    uStep->SetMobilityConfig(mob);
    uStep->UpdatePosition(1.0);
    auto velStep = uStep->GetVelocityEcef();
    double speedMagStep = std::sqrt(velStep.x * velStep.x + velStep.y * velStep.y + velStep.z * velStep.z);
    EXPECT_NEAR(speedMagStep, 250.0, 2.0); // 248.33 m/s on WGS-84 oblate ellipsoid
}

TEST(UserEngineTest, WaypointTrajectoryInterpolation) {
    NtnSim::Utils::GeodeticPosition p0{37.0, -122.0, 1000.0};
    NtnSim::Utils::GeodeticPosition p1{38.0, -120.0, 2000.0};

    auto user = std::make_shared<NtnSim::User::User>("u-waypoint", p0);
    NtnSim::User::MobilityConfig mob;
    mob.type = NtnSim::User::MobilityType::WaypointTrajectory;
    mob.waypoints = {
        {0.0, p0},
        {100.0, p1}
    };
    user->SetMobilityConfig(mob);

    // Midpoint at t = 50.0s
    user->UpdatePosition(50.0);
    auto midPos = user->GetPosition();
    EXPECT_NEAR(midPos.latitude, 37.5, 1e-4);
    EXPECT_NEAR(midPos.longitude, -121.0, 1e-4);
    EXPECT_NEAR(midPos.altitude, 1500.0, 1e-4);

    // End point at t = 100.0s
    user->UpdatePosition(100.0);
    auto endPos = user->GetPosition();
    EXPECT_NEAR(endPos.latitude, 38.0, 1e-4);
    EXPECT_NEAR(endPos.longitude, -120.0, 1e-4);
    EXPECT_NEAR(endPos.altitude, 2000.0, 1e-4);
}

TEST(UserEngineTest, UniformFleetGeneration) {
    NtnSim::User::UserEngine engine;

    // Generate 50 UEs across California bounding box: [32, 42] lat, [-124, -114] lon
    auto fleet = engine.GenerateUniformFleet("ca-ue", 50, 32.0, 42.0, -124.0, -114.0, NtnSim::User::UeClass::Vehicular);
    EXPECT_EQ(fleet.size(), 50u);
    EXPECT_EQ(engine.GetUserCount(), 50u);

    std::set<std::string> ids;
    for (const auto& u : fleet) {
        EXPECT_EQ(u->GetUeClass(), NtnSim::User::UeClass::Vehicular);
        auto p = u->GetPosition();
        EXPECT_GE(p.latitude, 32.0);
        EXPECT_LE(p.latitude, 42.0);
        EXPECT_GE(p.longitude, -124.0);
        EXPECT_LE(p.longitude, -114.0);
        ids.insert(u->GetId());
    }
    // All IDs must be unique
    EXPECT_EQ(ids.size(), 50u);
}

TEST(UserEngineTest, ClusteredFleetGeneration) {
    NtnSim::User::UserEngine engine;

    NtnSim::Utils::GeodeticPosition tokyo{35.6762, 139.6503, 10.0};
    double radiusKm = 50.0;

    auto fleet = engine.GenerateClusteredFleet("tokyo-ue", 30, tokyo, radiusKm, NtnSim::User::UeClass::Handheld);
    EXPECT_EQ(fleet.size(), 30u);
    EXPECT_EQ(engine.GetUserCount(), 30u);

    auto centerEcef = NtnSim::Utils::MathUtils::GeodeticToEcef(tokyo);

    for (const auto& u : fleet) {
        EXPECT_EQ(u->GetUeClass(), NtnSim::User::UeClass::Handheld);
        auto uEcef = u->GetEcefPosition();
        double distMeters = NtnSim::Utils::MathUtils::CalculateDistance(centerEcef, uEcef);
        // Distance must be within radius plus small curvature tolerance
        EXPECT_LE(distMeters, (radiusKm + 1.0) * 1000.0);
    }
}

TEST(UserEngineTest, EventSchedulerMobilityLoop) {
    NtnSim::User::UserEngine engine;
    NtnSim::Core::EventScheduler scheduler;

    // Add 2 moving users
    NtnSim::Utils::GeodeticPosition p0{0.0, 0.0, 0.0};
    auto u1 = std::make_shared<NtnSim::User::User>("u1", p0);
    auto u2 = std::make_shared<NtnSim::User::User>("u2", p0);

    NtnSim::User::MobilityConfig mob1;
    mob1.type = NtnSim::User::MobilityType::LinearGreatCircle;
    mob1.speedMps = 100.0;
    mob1.bearingDeg = 0.0; // North
    u1->SetMobilityConfig(mob1);

    NtnSim::User::MobilityConfig mob2;
    mob2.type = NtnSim::User::MobilityType::LinearGreatCircle;
    mob2.speedMps = 100.0;
    mob2.bearingDeg = 90.0; // East
    u2->SetMobilityConfig(mob2);

    engine.AddUser(u1);
    engine.AddUser(u2);

    // Schedule updates every 1.0s up to 5.0s (5 movement ticks)
    engine.ScheduleMobilityUpdates(1.0, 5.0, scheduler);

    size_t eventTicks = 0;
    while (!scheduler.IsEmpty()) {
        scheduler.ProcessNextEvent();
        eventTicks++;
    }

    EXPECT_EQ(eventTicks, 5u);

    // u1 moved North (latitude > 0, longitude ~= 0)
    EXPECT_GT(u1->GetPosition().latitude, 0.0);
    EXPECT_NEAR(u1->GetPosition().longitude, 0.0, 1e-4);

    // u2 moved East (longitude > 0, latitude ~= 0)
    EXPECT_GT(u2->GetPosition().longitude, 0.0);
    EXPECT_NEAR(u2->GetPosition().latitude, 0.0, 1e-4);
}
