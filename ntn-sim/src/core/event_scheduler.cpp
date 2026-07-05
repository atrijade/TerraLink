#include "ntn-sim/core/event_scheduler.h"

namespace NtnSim::Core {

EventScheduler::EventScheduler() : m_currentTime(0.0), m_nextEventId(0) {}

EventScheduler::~EventScheduler() {}

std::shared_ptr<Event> EventScheduler::ScheduleEvent(double timestamp, int priority, EventType type, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t id = ++m_nextEventId;
    auto event = std::make_shared<Event>(id, timestamp, priority, type, callback);
    m_queue.push(event);
    m_eventMap[id] = event;
    return event;
}

void EventScheduler::CancelEvent(uint64_t eventId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_eventMap.find(eventId);
    if (it != m_eventMap.end()) {
        it->second->Cancel();
    }
}

bool EventScheduler::ProcessNextEvent() {
    std::shared_ptr<Event> nextEvent;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) {
            nextEvent = m_queue.top();
            m_queue.pop();
            m_eventMap.erase(nextEvent->GetId());

            if (!nextEvent->IsCancelled()) {
                m_currentTime = nextEvent->GetTimestamp();
                break;
            }
            nextEvent = nullptr;
        }
    }

    if (nextEvent) {
        nextEvent->Execute();
        return true;
    }
    return false;
}

double EventScheduler::GetCurrentTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentTime;
}

bool EventScheduler::IsEmpty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

void EventScheduler::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
    m_eventMap.clear();
    m_currentTime = 0.0;
    m_nextEventId = 0;
}

} // namespace NtnSim::Core
