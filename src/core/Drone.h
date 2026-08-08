#pragma once

#include "core/GameConfig.h"
#include "core/math/Vec3.h"

namespace drone {

// Estado del dron. La fisica vive en PhysicsManager (rp3d); esta clase
// solo mantiene el estado de juego (empuje, bateria) y expone getters
// que GameController usa para sincronizar con el motor de fisica.
class Drone {
public:
    explicit Drone(const GameConfig& cfg) : m_config(cfg), m_battery(cfg.batteryMax) {}

    void setThrustInput(const Vec3& input);
    Vec3 thrustInput() const { return m_thrustInput; }

    Vec3 position() const { return m_position; }
    Vec3 velocity() const { return m_velocity; }
    float battery() const { return m_battery; }
    bool hasBattery() const { return m_battery > 0.0f; }

    // Lo decide el contacto real con el suelo, no la altura: con la esfera
    // simulada por rp3d, comparar la posición contra cero daba siempre
    // false y el fin de partida por batería agotada no llegaba nunca.
    bool isGrounded() const { return m_grounded; }
    void setGrounded(bool grounded) { m_grounded = grounded; }

    // Actitud medida, en radianes. Convención del simulador (+X derecha,
    // +Y arriba, +Z adelante):
    //   roll  > 0  ⇒ lado derecho ARRIBA   (giro sobre el eje Z)
    //   pitch > 0  ⇒ morro ARRIBA          (giro sobre el eje X)
    //   yaw        ⇒ rumbo, medido sobre el eje Y
    // Sin esto el PID angular no tiene nada que corregir.
    float roll() const { return m_roll; }
    float pitch() const { return m_pitch; }
    float yaw() const { return m_yaw; }
    void setAttitude(float roll, float pitch, float yaw) {
        m_roll = roll;
        m_pitch = pitch;
        m_yaw = yaw;
    }

    void setPosition(const Vec3& p) { m_position = p; }
    void setVelocity(const Vec3& v) { m_velocity = v; }
    void drainBattery(float amount);
    void reset();

private:
    const GameConfig& m_config;
    Vec3 m_position;
    Vec3 m_velocity;
    Vec3 m_thrustInput;
    float m_battery;
    bool m_grounded = true;
    float m_roll = 0.0f;
    float m_pitch = 0.0f;
    float m_yaw = 0.0f;
};

}  // namespace drone
