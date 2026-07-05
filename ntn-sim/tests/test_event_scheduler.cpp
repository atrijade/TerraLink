#include <gtest/gtest.h>
#include "ntn-sim/core/event_scheduler.h"
#include <vector>

TEST(EventSchedulerTest, SchedulingAndExecutionOrdering) {
    NtnSim::Core::EventScheduler scheduler;
    
    std::vector<int> executionOrder;
    
    // Schedule event at time 2.0
    scheduler.ScheduleEvent(2.0, 1, NtnSim::Core::EventType::MoveSatellite, [&executionOrder]() {
        executionOrder.push_back(2);
    });
    
    // Schedule event at time 1.0 (should fire first)
    scheduler.ScheduleEvent(1.0, 1, NtnSim::Core::EventType::MoveUser, [&executionOrder]() {
        executionOrder.push_back(1);
    });
    
    // Schedule event at time 2.0 with higher priority (should fire before the time 2.0, priority 1 event)
    scheduler.ScheduleEvent(2.0, 10, NtnSim::Core::EventType::Handover, [&executionOrder]() {
        executionOrder.push_back(3);
    });

    EXPECT_FALSE(scheduler.IsEmpty());

    // Process event 1 (time 1.0)
    EXPECT_TRUE(scheduler.ProcessNextEvent());
    EXPECT_EQ(scheduler.GetCurrentTime(), 1.0);
    
    // Process event 2 (time 2.0, priority 10)
    EXPECT_TRUE(scheduler.ProcessNextEvent());
    EXPECT_EQ(scheduler.GetCurrentTime(), 2.0);
    
    // Process event 3 (time 2.0, priority 1)
    EXPECT_TRUE(scheduler.ProcessNextEvent());
    EXPECT_EQ(scheduler.GetCurrentTime(), 2.0);
    
    EXPECT_TRUE(scheduler.IsEmpty());
    
    std::vector<int> expected = {1, 3, 2};
    EXPECT_EQ(executionOrder, expected);
}

TEST(EventSchedulerTest, EventCancellation) {
    NtnSim::Core::EventScheduler scheduler;
    bool fired = false;
    
    auto event = scheduler.ScheduleEvent(1.0, 1, NtnSim::Core::EventType::UpdateCoverage, [&fired]() {
        fired = true;
    });
    
    scheduler.CancelEvent(event->GetId());
    
    // ProcessNextEvent should skip the cancelled event
    EXPECT_FALSE(scheduler.ProcessNextEvent());
    EXPECT_FALSE(fired);
    EXPECT_TRUE(scheduler.IsEmpty());
}
