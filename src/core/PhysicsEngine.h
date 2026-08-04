#pragma once

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/GameConfig.h"

namespace drone {

class PhysicsEngine {
public:
    PhysicsEngine(const GameConfig& cfg, EventBus& bus) : m_config(cfg), m_bus(bus) {}

    void step(Drone& drone, const Environment& env, float dt);

private:
    void resolveGround(Drone& drone);
    void resolveBounds(Drone& drone);
    void resolveObstacles(Drone& drone, const Environment& env);
    void updateBattery(Drone& drone, const Vec3& thrust, float dt);

    const GameConfig& m_config;
    EventBus& m_bus;
};

}  // namespace drone
