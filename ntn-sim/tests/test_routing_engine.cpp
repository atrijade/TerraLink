#include <gtest/gtest.h>
#include "ntn-sim/routing/routing_engine.h"

TEST(RoutingEngineTest, ResolveNextHopSkeleton) {
    NtnSim::Routing::RoutingEngine engine;
    
    std::vector<std::string> route = engine.CalculateRoute("sat-1", "gw-0");
    ASSERT_EQ(route.size(), 2);
    EXPECT_EQ(route.front(), "sat-1");
    EXPECT_EQ(route.back(), "gw-0");
    
    std::string nextHop = engine.ResolveNextHop("sat-1", "gw-0");
    EXPECT_EQ(nextHop, "gw-0");
}
