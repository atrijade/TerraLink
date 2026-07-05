#include "ntn-sim/core/event.h"

namespace NtnSim::Core {

Event::Event(uint64_t id, double timestamp, int priority, EventType type, std::function<void()> callback)
    : m_id(id), m_timestamp(timestamp), m_priority(priority), m_type(type), m_callback(callback) {}

Event::~Event() {}

uint64_t Event::GetId() const {
    return m_id;
}

double Event::GetTimestamp() const {
    return m_timestamp;
}

int Event::GetPriority() const {
    return m_priority;
}

EventType Event::GetType() const {
    return m_type;
}

void Event::Execute() const {
    if (m_callback && !m_isCancelled) {
        m_callback();
    }
}

void Event::Cancel() {
    m_isCancelled = true;
}

bool Event::IsCancelled() const {
    return m_isCancelled;
}

} // namespace NtnSim::Core
