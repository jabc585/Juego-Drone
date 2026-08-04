#pragma once

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/PhysicsEngine.h"
#include "core/WorldState.h"

namespace drone {

// Agrega dron + entorno + física y posee el bus de eventos de la simulación.
class World {
public:
    World();

    void step(float dt);
    void reset();

    void setThrustInput(const Vec3& input) { m_drone.setThrustInput(input); }

    Drone& drone() { return m_drone; }
    const Drone& drone() const { return m_drone; }
    Environment& environment() { return m_environment; }
    const Environment& environment() const { return m_environment; }
    EventBus& events() { return m_bus; }
    float simTime() const { return m_simTime; }

    // Rellena la parte de simulación del snapshot; GameController añade
    // progresión y estado de la máquina de estados.
    WorldState snapshot() const;

private:
    EventBus m_bus;
    Drone m_drone;
    Environment m_environment;
    PhysicsEngine m_physics;
    float m_simTime = 0.0f;
};

}  // namespace drone
