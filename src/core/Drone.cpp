#include "core/Drone.h"

#include <algorithm>

namespace drone {

namespace {
float clampAxis(float v) {
    return std::max(-1.0f, std::min(1.0f, v));
}
}  // namespace

void Drone::setThrustInput(const Vec3& input) {
    m_thrustInput = {clampAxis(input.x), clampAxis(input.y), clampAxis(input.z)};
}

void Drone::drainBattery(float amount) {
    m_battery = std::max(0.0f, std::min(m_config.batteryMax, m_battery - amount));
}

void Drone::reset() {
    m_position = {};
    m_velocity = {};
    m_thrustInput = {};
    m_battery = m_config.batteryMax;
    m_grounded = true;
    m_roll = m_pitch = m_yaw = 0.0f;
}

}  // namespace drone
