#include <gtest/gtest.h>
#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/api/rest_api.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/user/user.h"

TEST(SimulationControllerTest, InitialState) {
    NtnSim::Core::SimulationController controller;
    EXPECT_EQ(controller.GetScheduler(), nullptr);
    
    controller.InitializeEngines();
    EXPECT_NE(controller.GetScheduler(), nullptr);
    EXPECT_NE(controller.GetOrbitEngine(), nullptr);
    EXPECT_NE(controller.GetUserEngine(), nullptr);
    EXPECT_NE(controller.GetCoverageEngine(), nullptr);
    EXPECT_NE(controller.GetSignalEngine(), nullptr);
    EXPECT_NE(controller.GetHandoverEngine(), nullptr);
    EXPECT_NE(controller.GetPacketEngine(), nullptr);
    EXPECT_NE(controller.GetRoutingEngine(), nullptr);
    EXPECT_NE(controller.GetGateway(), nullptr);
    EXPECT_NE(controller.GetGatewayEngine(), nullptr);
    EXPECT_NE(controller.GetMetricsEngine(), nullptr);
    EXPECT_NE(controller.GetRestApi(), nullptr);

    // Verify RestApi is wired to SimulationController and underlying engines
    auto* api = controller.GetRestApi();
    ASSERT_NE(api, nullptr);
    EXPECT_EQ(api->GetSimulationController(), &controller);
    EXPECT_EQ(api->GetOrbitEngine(), controller.GetOrbitEngine());
    EXPECT_EQ(api->GetRoutingEngine(), controller.GetRoutingEngine());
    EXPECT_EQ(api->GetGatewayEngine(), controller.GetGatewayEngine());
    EXPECT_EQ(api->GetGateway(), controller.GetGateway());
    EXPECT_EQ(api->GetMetricsEngine(), controller.GetMetricsEngine());
    EXPECT_EQ(api->GetUserEngine(), controller.GetUserEngine());
    EXPECT_EQ(api->GetCoverageEngine(), controller.GetCoverageEngine());
    EXPECT_EQ(api->GetPacketEngine(), controller.GetPacketEngine());
}

TEST(RestApiTest, DirectControllerWiringAndLifecycle) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    NtnSim::Api::RestApi api(&controller);
    EXPECT_EQ(api.GetSimulationController(), &controller);
    EXPECT_EQ(api.GetOrbitEngine(), controller.GetOrbitEngine());
    EXPECT_EQ(api.GetRoutingEngine(), controller.GetRoutingEngine());
    EXPECT_EQ(api.GetGatewayEngine(), controller.GetGatewayEngine());
    EXPECT_EQ(api.GetMetricsEngine(), controller.GetMetricsEngine());

    EXPECT_FALSE(api.IsRunning());
    EXPECT_TRUE(api.StartServer(9090));
    EXPECT_TRUE(api.IsRunning());
    EXPECT_EQ(api.GetPort(), 9090);

    api.StopServer();
    EXPECT_FALSE(api.IsRunning());
}

// =========================================================================
// Step 2: REST JSON Serializer Tests
// =========================================================================

TEST(RestApiTest, HandleGetSatellites_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    // Generate a mini Walker constellation (2 planes x 2 sats = 4 sats)
    controller.GetOrbitEngine()->GenerateWalkerConstellation(2, 2, 600.0, 53.0);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetSatellites();

    EXPECT_FALSE(json.empty());
    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"id\":\"sat-p0-s0\""), std::string::npos);
    EXPECT_NE(json.find("\"lat\":"), std::string::npos);
    EXPECT_NE(json.find("\"lon\":"), std::string::npos);
    EXPECT_NE(json.find("\"altKm\":"), std::string::npos);
    EXPECT_NE(json.find("\"ecef\":"), std::string::npos);
    EXPECT_NE(json.find("\"velKms\":"), std::string::npos);
}

TEST(RestApiTest, HandleGetTopology_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    controller.GetRoutingEngine()->AddLink("sat-0", "sat-1", 2400.0e3, false, 10.0e9);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetTopology();

    EXPECT_NE(json.find("{\"links\":["), std::string::npos);
    EXPECT_NE(json.find("\"src\":\"sat-0\""), std::string::npos);
    EXPECT_NE(json.find("\"dst\":\"sat-1\""), std::string::npos);
    EXPECT_NE(json.find("\"distanceKm\":2400.000"), std::string::npos);
    EXPECT_NE(json.find("\"dataRateGbps\":10.000"), std::string::npos);
    EXPECT_NE(json.find("\"active\":true"), std::string::npos);
}

TEST(RestApiTest, HandleGetGateways_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto* gw = controller.GetGateway();
    ASSERT_NE(gw, nullptr);
    gw->ConnectSatellite("sat-feeder-1");
    gw->ReceivePacket(101, 0.020, 1500, 0.0);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetGateways();

    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"id\":\"gw-0\""), std::string::npos);
    EXPECT_NE(json.find("\"connectedSat\":\"sat-feeder-1\""), std::string::npos);
    EXPECT_NE(json.find("\"hasActiveFeeder\":true"), std::string::npos);
    EXPECT_NE(json.find("\"throughputMbps\":"), std::string::npos);
    EXPECT_NE(json.find("\"jitterMs\":"), std::string::npos);
    EXPECT_NE(json.find("\"feederProfile\":"), std::string::npos);
}

TEST(RestApiTest, HandleGetUsers_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto user = std::make_shared<NtnSim::User::User>("ue-alpha", NtnSim::Utils::GeodeticPosition{37.7749, -122.4194, 0.0});
    user->SetServingSatelliteId("sat-serving-1");
    controller.GetUserEngine()->AddUser(user);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetUsers();

    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"id\":\"ue-alpha\""), std::string::npos);
    EXPECT_NE(json.find("\"servingSat\":\"sat-serving-1\""), std::string::npos);
    EXPECT_NE(json.find("\"lat\":37.774900"), std::string::npos);
}

TEST(RestApiTest, HandleGetMetrics_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto* metrics = controller.GetMetricsEngine();
    metrics->RecordPacketSent(1, 1500, 0.0);
    metrics->RecordPacketReceived(1, 0.025, 0.025);
    metrics->RecordHandover("ue-1", "sat-1", "sat-2", 0.05, true, 0.035);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetMetrics();

    EXPECT_NE(json.find("\"simTime\":"), std::string::npos);
    EXPECT_NE(json.find("\"traffic\":{"), std::string::npos);
    EXPECT_NE(json.find("\"packetsSent\":1"), std::string::npos);
    EXPECT_NE(json.find("\"packetsDelivered\":1"), std::string::npos);
    EXPECT_NE(json.find("\"pdrPercent\":100.0000"), std::string::npos);
    EXPECT_NE(json.find("\"handovers\":{"), std::string::npos);
    EXPECT_NE(json.find("\"totalSuccesses\":1"), std::string::npos);
    EXPECT_NE(json.find("\"radio\":{"), std::string::npos);
}
