#pragma once

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/GameConfig.h"
#include "core/PhysicsEngine.h"
#include "core/WorldState.h"

namespace drone {

class World {
public:
    explicit World(const GameConfig& cfg);

    void step(float dt);
    void reset();
    // Restaura el reloj de simulación (y la dificultad derivada) al cargar.
    void restoreSimTime(float simTime);

    void setThrustInput(const Vec3& input) { m_drone.setThrustInput(input); }

    Drone& drone() { return m_drone; }
    const Drone& drone() const { return m_drone; }
    Environment& environment() { return m_environment; }
    const Environment& environment() const { return m_environment; }
    EventBus& events() { return m_bus; }
    float simTime() const { return m_simTime; }

    WorldState snapshot() const;

private:
    EventBus m_bus;
    Drone m_drone;
    Environment m_environment;
    PhysicsEngine m_physics;
    float m_simTime = 0.0f;
};

}  // namespace drone
