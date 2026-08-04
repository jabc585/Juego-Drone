#pragma once

#include "core/Config.h"
#include "core/math/Vec3.h"

namespace drone {

// Estado del dron. La integración de fuerzas vive en PhysicsEngine (PLAN2.md R4):
// esta clase no contiene lógica de movimiento, solo estado y sus invariantes.
class Drone {
public:
    // Entrada de empuje normalizada: cada eje se recorta a [-1, 1].
    void setThrustInput(const Vec3& input);
    Vec3 thrustInput() const { return m_thrustInput; }

    Vec3 position() const { return m_position; }
    Vec3 velocity() const { return m_velocity; }
    float battery() const { return m_battery; }
    bool hasBattery() const { return m_battery > 0.0f; }
    bool isGrounded() const { return m_position.y <= 0.0f; }

    void setPosition(const Vec3& p) { m_position = p; }
    void setVelocity(const Vec3& v) { m_velocity = v; }

    // Resta batería; nunca baja de 0 ni sube de kBatteryMax.
    void drainBattery(float amount);

    // Apoya el dron en el suelo anulando la velocidad vertical.
    void clampToGround();

    void reset();

private:
    Vec3 m_position;
    Vec3 m_velocity;
    Vec3 m_thrustInput;
    float m_battery = config::kBatteryMax;
};

}  // namespace drone
