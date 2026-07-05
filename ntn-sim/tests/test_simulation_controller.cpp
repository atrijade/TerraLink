#include <gtest/gtest.h>
#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/core/event_scheduler.h"

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
    EXPECT_NE(controller.GetMetricsEngine(), nullptr);
    EXPECT_NE(controller.GetRestApi(), nullptr);
}
