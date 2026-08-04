#include "core/EventBus.h"

namespace drone {

void EventBus::subscribe(EventType type, Handler handler) {
    m_handlers[type].push_back(std::move(handler));
}

void EventBus::subscribeAll(Handler handler) {
    m_globalHandlers.push_back(std::move(handler));
}

void EventBus::publish(const Event& event) {
    auto it = m_handlers.find(event.type);
    if (it != m_handlers.end()) {
        for (auto& handler : it->second)
            handler(event);
    }
    for (auto& handler : m_globalHandlers)
        handler(event);
}

}  // namespace drone
