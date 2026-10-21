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

TEST(PacketEngineTest, PacketHeaderMetadataAndTtlExpiry) {
    NtnSim::Packet::PacketEngine engine;

    // Packet with ID 2001, 1000 bytes, created at t=1.0s, qos=1 (voice), ttl=3, maxLifetime=2.0s
    auto pkt = engine.GeneratePacket(2001, 1000, 1.0, "user-10", "gw-0", 1, 3, 2.0);

    EXPECT_EQ(pkt->GetQosClass(), 1u);
    EXPECT_EQ(pkt->GetTtl(), 3u);
    EXPECT_DOUBLE_EQ(pkt->GetMaxLifetimeSeconds(), 2.0);

    // Initial hop history contains the source node
    EXPECT_EQ(pkt->GetHopCount(), 0u);
    EXPECT_EQ(pkt->GetHopHistory().front(), "user-10");

    // Trace route hops: user -> sat-0 -> sat-1 -> gw-0
    pkt->AddHop("sat-p0-s0");
    pkt->AddHop("sat-p0-s1");
    pkt->AddHop("gw-0");
    EXPECT_EQ(pkt->GetHopCount(), 3u);
    EXPECT_EQ(pkt->GetHopHistory().size(), 4u);

    // Test TTL decrement
    EXPECT_TRUE(pkt->DecrementTtl()); // 3 -> 2
    EXPECT_EQ(pkt->GetTtl(), 2u);
    EXPECT_TRUE(pkt->DecrementTtl()); // 2 -> 1
    EXPECT_EQ(pkt->GetTtl(), 1u);
    EXPECT_FALSE(pkt->DecrementTtl()); // 1 -> 0
    EXPECT_EQ(pkt->GetTtl(), 0u);

    // Expired due to TTL == 0
    EXPECT_TRUE(pkt->IsExpired(1.5));

    // Test lifetime timeout expiry
    auto freshPkt = engine.GeneratePacket(2002, 500, 10.0, "u1", "gw1", 2, 64, 2.0);
    EXPECT_FALSE(freshPkt->IsExpired(11.5)); // elapsed 1.5s <= 2.0s
    EXPECT_TRUE(freshPkt->IsExpired(12.5));  // elapsed 2.5s > 2.0s
}

TEST(PacketEngineTest, DropTailPacketCountLimit) {
    NtnSim::Packet::PacketEngine engine;

    // Configure queue limit to max 3 packets
    engine.ConfigureQueue("sat-queue-1", 3, 10 * 1024 * 1024);
    EXPECT_EQ(engine.GetQueueConfig("sat-queue-1").maxPackets, 3u);

    auto p1 = engine.GeneratePacket(1, 1000, 0.0, "u", "gw");
    auto p2 = engine.GeneratePacket(2, 1000, 0.0, "u", "gw");
    auto p3 = engine.GeneratePacket(3, 1000, 0.0, "u", "gw");
    auto p4 = engine.GeneratePacket(4, 1000, 0.0, "u", "gw");

    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p1, 1.0));
    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p2, 1.0));
    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p3, 1.0));
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 3u);

    // 4th packet must be dropped (DropTail)
    EXPECT_FALSE(engine.EnqueuePacket("sat-queue-1", p4, 1.0));
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 3u);
    EXPECT_EQ(engine.GetDroppedPacketCount("sat-queue-1"), 1u);

    // Dequeue one, then enqueue p4 successfully
    auto popped = engine.DequeuePacket("sat-queue-1", 1.1);
    ASSERT_NE(popped, nullptr);
    EXPECT_EQ(popped->GetId(), 1u);
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 2u);

    EXPECT_TRUE(engine.EnqueuePacket("sat-queue-1", p4, 1.1));
    EXPECT_EQ(engine.GetQueueSize("sat-queue-1"), 3u);
}

TEST(PacketEngineTest, DropTailByteCapacityLimit) {
    NtnSim::Packet::PacketEngine engine;

    // Configure queue to max 3000 bytes
    engine.ConfigureQueue("byte-queue", 100, 3000);

    auto p1 = engine.GeneratePacket(10, 1500, 0.0, "u", "gw");
    auto p2 = engine.GeneratePacket(11, 1500, 0.0, "u", "gw");
    auto p3 = engine.GeneratePacket(12, 1000, 0.0, "u", "gw");

    EXPECT_TRUE(engine.EnqueuePacket("byte-queue", p1, 1.0));
    EXPECT_TRUE(engine.EnqueuePacket("byte-queue", p2, 1.0));
    EXPECT_EQ(engine.GetQueueSizeBytes("byte-queue"), 3000u);

    // p3 (1000 bytes) exceeds 3000 bytes capacity -> dropped
    EXPECT_FALSE(engine.EnqueuePacket("byte-queue", p3, 1.0));
    EXPECT_EQ(engine.GetDroppedPacketCount("byte-queue"), 1u);
    EXPECT_EQ(engine.GetQueueSizeBytes("byte-queue"), 3000u);
}

TEST(PacketEngineTest, QueueLatencyCalculation) {
    NtnSim::Packet::PacketEngine engine;

    auto pkt = engine.GeneratePacket(5001, 1500, 1.000, "user-1", "gw-0");

    // Enqueued at t = 1.200s
    EXPECT_TRUE(engine.EnqueuePacket("sat-buffer", pkt, 1.200));
    EXPECT_DOUBLE_EQ(pkt->GetEnqueueTime(), 1.200);

    // Dequeued at t = 1.235s (spent 35 ms in queue)
    auto dequeued = engine.DequeuePacket("sat-buffer", 1.235);
    ASSERT_NE(dequeued, nullptr);
    EXPECT_DOUBLE_EQ(dequeued->GetDepartureTime(), 1.235);
    EXPECT_NEAR(dequeued->GetQueueDelay(), 0.035, 1e-6); // 35 ms delay
    EXPECT_NEAR(engine.GetLastQueueDelay("sat-buffer"), 0.035, 1e-6);

    // Set arrival time and verify end-to-end latency
    dequeued->SetArrivalTime(1.250); // arrived at t = 1.250s, created at t = 1.000s
    EXPECT_NEAR(dequeued->GetEndToEndLatency(), 0.250, 1e-6); // 250 ms total
}
