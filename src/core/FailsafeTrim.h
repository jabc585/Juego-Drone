#pragma once

namespace drone {

// Failsafe: si el jugador deja de dar empuje durante `timeoutSeconds`, el
// control baja el gas suavemente desde el de hover hasta cero, en lugar de
// cortarlo de golpe.
//
// En el suelo no se activa, igual que en el original: un dron posado no
// tiene nada que aterrizar, y activarlo allí dejaría el failsafe enganchado
// para siempre.
struct Failsafe {
    float timeoutSeconds = 2.0f;
    // Fracción de empuje que se pierde por segundo durante el descenso.
    float descentPerSecond = 0.25f;

    bool active = false;
    float timeSinceInput = 0;
    float landingThrottle = 0;

    void reset();

    // Devuelve el throttle que debe usarse. `hoverThrottle` es el punto de
    // partida del descenso; `grounded` inhibe el failsafe.
    float compute(float inputThrottle, float hoverThrottle, bool grounded, float dt);
};

// Trims acumulativos de actitud (F1–F4). Se suman al SETPOINT, no a la
// salida del PID: así el término integral no pelea contra el trim.
struct AttitudeTrim {
    float pitchDeg = 0;
    float rollDeg = 0;

    // Un trim sin tope acaba en una consigna imposible de volar.
    static constexpr float kLimitDeg = 15.0f;

    void addPitch(float deg) { pitchDeg = clampTrim(pitchDeg + deg); }
    void addRoll(float deg) { rollDeg = clampTrim(rollDeg + deg); }
    void reset() { pitchDeg = rollDeg = 0; }

private:
    static float clampTrim(float v) {
        return v < -kLimitDeg ? -kLimitDeg : (v > kLimitDeg ? kLimitDeg : v);
    }
};

}  // namespace drone
