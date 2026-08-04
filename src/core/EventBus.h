#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "core/Events.h"

namespace drone {

// Pub/sub síncrono y tipado (PLAN2.md §7 R7). Los handlers se invocan en
// orden de suscripción, en el mismo hilo que publish().
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(EventType type, Handler handler);
    // Recibe todos los eventos; útil para reenviarlos al frontend.
    void subscribeAll(Handler handler);
    void publish(const Event& event);

private:
    std::unordered_map<EventType, std::vector<Handler>> m_handlers;
    std::vector<Handler> m_globalHandlers;
};

}  // namespace drone
