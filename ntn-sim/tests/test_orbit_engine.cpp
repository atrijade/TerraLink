#include <gtest/gtest.h>
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/orbit/satellite.h"

TEST(OrbitEngineTest, AddAndRetrieveSatellite) {
    NtnSim::Orbit::OrbitEngine engine;
    
    NtnSim::Orbit::OrbitState state;
    NtnSim::Utils::Position3D pos{100.0, 200.0, 300.0};
    state.SetEcefPosition(pos);
    
    auto sat = std::make_shared<NtnSim::Orbit::Satellite>("sat-1", state);
    engine.AddSatellite(sat);
    
    auto retrieved = engine.GetSatellite("sat-1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->GetId(), "sat-1");
    EXPECT_DOUBLE_EQ(retrieved->GetOrbitState().GetEcefPosition().x, 100.0);
    
    EXPECT_EQ(engine.GetSatellite("non-existent"), nullptr);
    EXPECT_EQ(engine.GetAllSatellites().size(), 1);
}
