#pragma once

namespace drone {

// Altitude Hold: P puro sobre el error de altitud, con el empuje de hover
// como prealimentación (cortex-main usa ALT_KI = 0 y ALT_KD = 0).
//
// hoverThrust NO es una constante: lo calcula World a partir de la masa y la
// gravedad reales, para que cambiar la masa en el TOML no descuadre el hover.
struct AltitudeHold {
    float kp = 1.5f;
    float hoverThrust = 0.5f;     // fracción del empuje máximo; la fija World
    float maxCorrection = 0.15f;  // corrección máxima sobre el throttle

    bool engaged = false;
    float targetAltitude = 0;

    void reset();
    void toggle(float currentAltitude);
    float compute(float currentAltitude) const;
};

// Yaw Hold: con el mando de guiñada en reposo, bloquea el rumbo. Devuelve
// una TASA de giro (rad/s), que es lo que consume el lazo de velocidad del
// PID — no un ángulo.
struct YawHold {
    float holdKp = 2.0f;
    float deadzone = 0.1f;  // por debajo de esto el mando se considera suelto

    float targetHeading = 0;
    bool targetSet = false;

    float compute(float stickInput, float currentHeading);
    void reset();
};

}  // namespace drone
