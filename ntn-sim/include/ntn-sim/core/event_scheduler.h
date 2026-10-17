#pragma once

#include "ntn-sim/core/event.h"
#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace NtnSim::Core {

/**
 * @brief Discrete event scheduler keeping track of scheduled events.
 */
class EventScheduler {
public:
    EventScheduler();
    ~EventScheduler();

    /**
     * @brief Schedules a callback event in the simulation timeline.
     * @param timestamp Execution simulation time.
     * @param priority Order resolution for simultaneous events.
     * @param type Event type.
     * @param callback std::function to trigger.
     * @return Shared pointer to the scheduled Event.
     */
    std::shared_ptr<Event> ScheduleEvent(double timestamp, int priority, EventType type, std::function<void()> callback);

    /**
     * @brief Marks a scheduled event as cancelled using its unique ID.
     */
    void CancelEvent(uint64_t eventId);

    /**
     * @brief Pops and executes the next non-cancelled event.
     * @return True if an event was popped and run.
     */
    bool ProcessNextEvent();

    /**
     * @brief Returns the current virtual simulation clock time.
     */
    double GetCurrentTime() const;

    /**
     * @brief Returns true if there are no events in the queue.
     */
    bool IsEmpty() const;

    /**
     * @brief Clears all pending events in the scheduler.
     */
    void Clear();

private:
    double m_currentTime{0.0};
    uint64_t m_nextEventId{0};
    
    // Priority queue of events
    std::priority_queue<std::shared_ptr<Event>, std::vector<std::shared_ptr<Event>>, EventComparator> m_queue;
    
    // Maps event ID to the event for O(1) cancelation capability
    std::unordered_map<uint64_t, std::shared_ptr<Event>> m_eventMap;
    
    mutable std::mutex m_mutex;
};

} // namespace NtnSim::Core
