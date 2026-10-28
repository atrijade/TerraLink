#include <gtest/gtest.h>
#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/api/rest_api.h"

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
