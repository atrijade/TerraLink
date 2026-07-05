#include <gtest/gtest.h>
#include "ntn-sim/packet/packet_engine.h"
#include "ntn-sim/packet/packet.h"

TEST(PacketEngineTest, EnqueueAndDequeuePackets) {
    NtnSim::Packet::PacketEngine engine;
    
    auto pkt = engine.GeneratePacket(1001, 1500, 0.0, "user-1", "gw-0");
    EXPECT_EQ(pkt->GetId(), 1001);
    EXPECT_EQ(pkt->GetSizeBytes(), 1500);
    EXPECT_EQ(pkt->GetSourceNodeId(), "user-1");
    
    engine.EnqueuePacket("buffer-1", pkt);
    EXPECT_EQ(engine.GetQueueSize("buffer-1"), 1);
    
    auto popped = engine.DequeuePacket("buffer-1");
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(popped->GetId(), 1001);
    EXPECT_EQ(engine.GetQueueSize("buffer-1"), 0);
}
