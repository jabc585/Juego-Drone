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
    m_battery = std::max(0.0f, std::min(config::kBatteryMax, m_battery - amount));
}

void Drone::clampToGround() {
    if (m_position.y < 0.0f)
        m_position.y = 0.0f;
    if (m_velocity.y < 0.0f)
        m_velocity.y = 0.0f;
}

void Drone::reset() {
    m_position = {};
    m_velocity = {};
    m_thrustInput = {};
    m_battery = config::kBatteryMax;
}

}  // namespace drone
