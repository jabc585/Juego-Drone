#pragma once

#include <functional>
#include <vector>

#include "core/math/Vec3.h"
#include "physics/PhysicsTypes.h"

namespace drone::physics {

struct ContactEvent {
    BodyId bodyA;
    BodyId bodyB;
    Vec3 point;
    Vec3 normal;
    float penetration;
    float impulse;  // estimado desde velocidades pre-paso
};

// Recibe los eventos de colision que rp3d produce durante world->update().
// Los almacena para que el pipeline los despache en PostStep (nunca dentro
// del callback, donde crear/destruir cuerpos es UB).
class ContactListener {
public:
    using Callback = std::function<void(const ContactEvent&)>;

    void onContact(const ContactEvent& e) { m_events.push_back(e); }
    void setCallback(Callback cb) { m_callback = std::move(cb); }

    void dispatch() {
        if (!m_callback) {
            m_events.clear();
            return;
        }
        for (const auto& e : m_events)
            m_callback(e);
        m_events.clear();
    }

    size_t pendingCount() const { return m_events.size(); }

private:
    std::vector<ContactEvent> m_events;
    Callback m_callback;
};

}  // namespace drone::physics
