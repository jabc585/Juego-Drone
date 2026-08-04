#include "core/World.h"

namespace drone {

World::World() : m_physics(m_bus) {}

void World::step(float dt) {
    m_environment.step(dt);
    m_physics.step(m_drone, m_environment, dt);
    m_simTime += dt;
}

void World::reset() {
    m_drone.reset();
    m_environment.reset();
    m_simTime = 0.0f;
}

WorldState World::snapshot() const {
    WorldState s;
    s.dronePosition = m_drone.position();
    s.droneVelocity = m_drone.velocity();
    s.wind = m_environment.wind();
    s.battery = m_drone.battery();
    s.difficulty = m_environment.difficulty();
    s.simTime = m_simTime;
    s.environmentName = m_environment.name();
    return s;
}

}  // namespace drone
