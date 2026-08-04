#pragma once

#include "core/GameConfig.h"
#include "core/math/Vec3.h"

namespace drone {

class Drone {
public:
    explicit Drone(const GameConfig& cfg) : m_config(cfg), m_battery(cfg.batteryMax) {}

    void setThrustInput(const Vec3& input);
    Vec3 thrustInput() const { return m_thrustInput; }

    Vec3 position() const { return m_position; }
    Vec3 velocity() const { return m_velocity; }
    float battery() const { return m_battery; }
    bool hasBattery() const { return m_battery > 0.0f; }
    bool isGrounded() const { return m_position.y <= 0.0f; }

    void setPosition(const Vec3& p) { m_position = p; }
    void setVelocity(const Vec3& v) { m_velocity = v; }
    void drainBattery(float amount);
    void clampToGround();
    void reset();

private:
    const GameConfig& m_config;
    Vec3 m_position;
    Vec3 m_velocity;
    Vec3 m_thrustInput;
    float m_battery;
};

}  // namespace drone
