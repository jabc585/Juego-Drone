#include "core/PhysicsEngine.h"

#include <algorithm>
#include <cmath>

namespace drone {

void PhysicsEngine::step(Drone& drone, const Environment& env, float dt) {
    const Vec3 thrust = drone.hasBattery() ? drone.thrustInput() * m_config.maxThrust : Vec3{};
    const Vec3 gravity{0.0f, -m_config.gravity * m_config.droneMass, 0.0f};
    const Vec3 air = (env.wind() - drone.velocity()) * m_config.dragCoefficient;

    const Vec3 accel = (thrust + gravity + air) * (1.0f / m_config.droneMass);

    drone.setVelocity(drone.velocity() + accel * dt);
    drone.setPosition(drone.position() + drone.velocity() * dt);

    resolveGround(drone);
    resolveBounds(drone);
    resolveObstacles(drone, env);
    updateBattery(drone, thrust, dt);
}

void PhysicsEngine::resolveGround(Drone& drone) {
    if (drone.position().y > 0.0f)
        return;
    const float impact = -drone.velocity().y;
    drone.clampToGround();
    if (impact > m_config.crashSpeed) {
        m_bus.publish({EventType::Collision, impact});
    }
}

void PhysicsEngine::resolveBounds(Drone& drone) {
    Vec3 p = drone.position();
    Vec3 v = drone.velocity();
    const auto clampAxis = [&](float& pos, float& vel, float lo, float hi) {
        if (pos < lo) {
            pos = lo;
            if (vel < 0)
                vel = 0;
        } else if (pos > hi) {
            pos = hi;
            if (vel > 0)
                vel = 0;
        }
    };
    clampAxis(p.x, v.x, -m_config.worldHalfExtent, m_config.worldHalfExtent);
    clampAxis(p.z, v.z, -m_config.worldHalfExtent, m_config.worldHalfExtent);
    clampAxis(p.y, v.y, 0.0f, m_config.maxAltitude);
    drone.setPosition(p);
    drone.setVelocity(v);
}

void PhysicsEngine::resolveObstacles(Drone& drone, const Environment& env) {
    const float r = m_config.droneRadius;
    for (const Obstacle& box : env.obstacles()) {
        const Vec3 half = box.size * 0.5f;
        const Vec3 d = drone.position() - box.center;
        const float penX = half.x + r - std::fabs(d.x);
        const float penY = half.y + r - std::fabs(d.y);
        const float penZ = half.z + r - std::fabs(d.z);
        if (penX <= 0.0f || penY <= 0.0f || penZ <= 0.0f)
            continue;

        const float speed = drone.velocity().length();
        Vec3 p = drone.position();
        Vec3 v = drone.velocity();
        if (penX <= penY && penX <= penZ) {
            p.x += (d.x >= 0 ? penX : -penX);
            v.x = 0;
        } else if (penY <= penX && penY <= penZ) {
            p.y += (d.y >= 0 ? penY : -penY);
            v.y = 0;
        } else {
            p.z += (d.z >= 0 ? penZ : -penZ);
            v.z = 0;
        }
        drone.setPosition(p);
        drone.setVelocity(v);
        m_bus.publish({EventType::Collision, speed});
    }
}

void PhysicsEngine::updateBattery(Drone& drone, const Vec3& thrust, float dt) {
    const float before = drone.battery();
    drone.drainBattery(thrust.length() * m_config.batteryPerNewton * dt);
    const float after = drone.battery();

    if (before > m_config.batteryLowThreshold && after <= m_config.batteryLowThreshold &&
        after > 0) {
        m_bus.publish({EventType::BatteryLow, after});
    }
    if (before > 0.0f && after <= 0.0f) {
        m_bus.publish({EventType::BatteryEmpty, 0.0f});
    }
}

}  // namespace drone
