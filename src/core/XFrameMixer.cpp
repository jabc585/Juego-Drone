#include "core/XFrameMixer.h"

#include <algorithm>

namespace drone {

XFrameMixer::Result XFrameMixer::compute(float throttle, float roll, float pitch, float yaw,
                                         float maxThrustTotal) const {
    const float maxPerMotor = maxThrustTotal / 4.0f;

    // Matriz de mezcla (cortex-main flight_controller.cpp:265-271).
    const float m1 = throttle - pitch + roll + yaw;  // trasero derecho, CCW
    const float m2 = throttle + pitch + roll - yaw;  // delantero derecho, CW
    const float m3 = throttle - pitch - roll - yaw;  // trasero izquierdo, CW
    const float m4 = throttle + pitch - roll + yaw;  // delantero izquierdo, CCW

    const auto scale = [maxPerMotor](float v) {
        return std::max(0.0f, std::min(maxPerMotor, v * maxPerMotor));
    };

    // X a 45°: la proyección del brazo sobre cada eje es arm·cos45.
    const float d = armLength * 0.70710678f;

    Result r;
    r.motors[0] = {scale(m4), {-d, 0, d}, false};  // delantero izquierdo
    r.motors[1] = {scale(m2), {d, 0, d}, true};    // delantero derecho
    r.motors[2] = {scale(m3), {-d, 0, -d}, true};  // trasero izquierdo
    r.motors[3] = {scale(m1), {d, 0, -d}, false};  // trasero derecho

    // Una hélice que gira en un sentido aplica al chasis el par contrario:
    // las CCW empujan el morro en sentido horario y viceversa. Con empuje
    // simétrico los cuatro se cancelan y el dron no guiña.
    for (const MotorOutput& m : r.motors)
        r.yawTorque += (m.clockwise ? -1.0f : 1.0f) * yawTorqueFactor * m.thrust;

    return r;
}

}  // namespace drone
