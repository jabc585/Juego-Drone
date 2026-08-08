#pragma once

#include "core/math/Vec3.h"

namespace drone {

// Quadcopter en X: 4 motores con la matriz de mezcla verificada contra
// cortex-main (flight_controller.cpp:265-271). Función pura: sin física,
// sin estado y sin I/O, para poder probar la tabla de signos sin motor.
//
// Convención del simulador (+X derecha, +Y arriba, +Z adelante):
//   pitch+ ⇒ suben los delanteros (M2, M4) ⇒ morro ARRIBA
//   roll+  ⇒ suben los derechos   (M1, M2) ⇒ lado derecho ARRIBA
//   yaw+   ⇒ suben los CCW        (M1, M4) ⇒ giro HORARIO por reacción
struct XFrameMixer {
    float armLength = 0.25f;  // m del centro de masa al motor
    // Par de reacción de la hélice por newton de empuje. Sin este término
    // el yaw no hace nada: los cuatro empujes son paralelos y ninguno
    // genera par sobre el eje vertical.
    float yawTorqueFactor = 0.02f;  // N·m por N

    struct MotorOutput {
        float thrust = 0;  // N, en [0, maxThrustTotal/4]
        Vec3 position;     // desplazamiento respecto al centro de masa (m)
        bool clockwise = false;
    };

    struct Result {
        MotorOutput motors[4];
        float yawTorque = 0;  // N·m sobre el eje vertical del cuerpo
    };

    // throttle ∈ [0,1]; roll/pitch/yaw son correcciones normalizadas.
    Result compute(float throttle, float roll, float pitch, float yaw, float maxThrustTotal) const;
};

}  // namespace drone
