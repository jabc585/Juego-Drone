#include "core/PhysicsEngine.h"

#include <algorithm>
#include <cmath>

#include "core/Config.h"

namespace drone {

using config::kBatteryLowThreshold;
using config::kBatteryPerNewton;
using config::kCrashSpeed;
using config::kDragCoefficient;
using config::kDroneMass;
using config::kDroneRadius;
using config::kGravity;
using config::kMaxAltitude;
using config::kMaxThrust;
using config::kWorldHalfExtent;

void PhysicsEngine::step(Drone& drone, const Environment& env, float dt) {
    // Sin batería no hay empuje (B9): el dron cae y solo queda planear.
    const Vec3 thrust = drone.hasBattery() ? drone.thrustInput() * kMaxThrust : Vec3{};
    const Vec3 gravity{0.0f, -kGravity * kDroneMass, 0.0f};
    // Un solo término aerodinámico: arrastra hacia la velocidad del viento y
    // frena el movimiento propio (da velocidad terminal y empuje de rachas).
    const Vec3 air = (env.wind() - drone.velocity()) * kDragCoefficient;

    const Vec3 accel = (thrust + gravity + air) * (1.0f / kDroneMass);

    // Euler semi-implícito: v primero, luego p (estable a 60 Hz, ADR-001).
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
    if (impact > kCrashSpeed) {
        m_bus.publish({EventType::Collision, impact});
    }
}

void PhysicsEngine::resolveBounds(Drone& drone) {
    Vec3 p = drone.position();
    Vec3 v = drone.velocity();
    const auto clampAxis = [](float& pos, float& vel, float lo, float hi) {
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
    clampAxis(p.x, v.x, -kWorldHalfExtent, kWorldHalfExtent);
    clampAxis(p.z, v.z, -kWorldHalfExtent, kWorldHalfExtent);
    clampAxis(p.y, v.y, 0.0f, kMaxAltitude);
    drone.setPosition(p);
    drone.setVelocity(v);
}

void PhysicsEngine::resolveObstacles(Drone& drone, const Environment& env) {
    for (const Obstacle& box : env.obstacles()) {
        const Vec3 half = box.size * 0.5f;
        const Vec3 d = drone.position() - box.center;
        const float penX = half.x + kDroneRadius - std::fabs(d.x);
        const float penY = half.y + kDroneRadius - std::fabs(d.y);
        const float penZ = half.z + kDroneRadius - std::fabs(d.z);
        if (penX <= 0.0f || penY <= 0.0f || penZ <= 0.0f)
            continue;

        const float speed = drone.velocity().length();
        Vec3 p = drone.position();
        Vec3 v = drone.velocity();
        // Empuja hacia fuera por el eje de menor penetración y anula esa
        // componente de velocidad.
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
    drone.drainBattery(thrust.length() * kBatteryPerNewton * dt);
    const float after = drone.battery();

    if (before > kBatteryLowThreshold && after <= kBatteryLowThreshold && after > 0) {
        m_bus.publish({EventType::BatteryLow, after});
    }
    if (before > 0.0f && after <= 0.0f) {
        m_bus.publish({EventType::BatteryEmpty, 0.0f});
    }
}

}  // namespace drone
