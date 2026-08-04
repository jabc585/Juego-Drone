#pragma once

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"

namespace drone {

// Único dueño de la integración de fuerzas (PLAN2.md R4): empuje, gravedad,
// arrastre/viento, suelo, límites del mundo, obstáculos AABB y batería.
class PhysicsEngine {
public:
    explicit PhysicsEngine(EventBus& bus) : m_bus(bus) {}

    void step(Drone& drone, const Environment& env, float dt);

private:
    void resolveGround(Drone& drone);
    void resolveBounds(Drone& drone);
    void resolveObstacles(Drone& drone, const Environment& env);
    void updateBattery(Drone& drone, const Vec3& thrust, float dt);

    EventBus& m_bus;
};

}  // namespace drone
