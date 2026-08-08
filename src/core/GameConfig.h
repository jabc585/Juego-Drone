#pragma once

namespace drone {

// Valores de configuracion con defaults de fabrica. Se construye con los
// valores en tiempo de compilacion; main puede sobreescribir desde
// assets/config/game.toml antes de inyectarla en GameController.
// Tests construyen instancias sin tocar ficheros ni I/O.
struct GameConfig {
    // --- Fisica ---
    // La gravedad ya no vive aqui: la aplica rp3d desde
    // PhysicsSettings::gravity ([physics.world] en game.toml). Mantener una
    // copia en GameConfig dejaba una clave que se podia editar sin efecto.
    float droneMass = 1.2f;
    float maxThrust = 25.0f;
    float dragCoefficient = 0.35f;
    float droneRadius = 0.4f;
    float crashSpeed = 8.0f;

    // --- Bateria ---
    float batteryMax = 100.0f;
    float batteryPerNewton = 0.02f;
    float batteryLowThreshold = 20.0f;

    // --- Bucle de juego ---
    float fixedTimestep = 1.0f / 60.0f;
    float maxFrameTime = 0.25f;
    float thrustPulseSeconds = 0.35f;

    // --- Progresion ---
    int xpPerLevelBase = 100;
    int unlockLevel = 3;
    float xpPerSecond = 5.0f;

    // --- Mundo ---
    float worldHalfExtent = 100.0f;
    float maxAltitude = 50.0f;

    // --- Entorno / viento ---
    float windBaseSpeed = 1.5f;
    float windSmoothing = 2.0f;
    float gustMinInterval = 2.0f;
    float gustMaxInterval = 6.0f;
    float difficultyRamp = 0.02f;
    float maxDifficulty = 10.0f;

    // Materiales (grafico.md §6.6)
    float droneFriction = 0.3f;
    float droneBounciness = 0.0f;  // sin rebote
    float obstacleFriction = 0.6f;
    float obstacleBounciness = 0.1f;
    float groundFriction = 0.8f;
    float groundBounciness = 0.0f;
    float linearDamping = 0.3f;  // amortiguacion lineal (0=sin friccion)
    float angularDamping = 0.5f;

    // Zonas de aterrizaje: cubos trigger que dan XP al posarse
    bool landingZonesEnabled = true;
    float landingZoneRadius = 2.0f;
    float landingZoneHeight = 0.5f;
    int landingZoneXP = 50;  // XP al aterrizar en una zona

    // --- Control de vuelo (portado de cortex-main) ---
    // Chasis y motores
    float armLength = 0.25f;          // m del centro de masa a cada motor
    float yawTorqueFactor = 0.02f;    // N·m de reacción por N de empuje
    float motorTimeConstant = 0.05f;  // s de respuesta del conjunto motor+ESC
    float droneInertia = 0.006f;      // kg·m², momento de un quad de 250 mm
    float maxTiltDeg = 20.0f;         // inclinación máxima que pide el mando

    // Ganancias del PID angular, EN LAS UNIDADES DEL ORIGINAL (grados y
    // cuentas PWM sobre 1997). World aplica la conversión a radianes y
    // fracción de empuje; así estos números se pueden comparar uno a uno
    // con los de cortex sin traducir nada a mano.
    float pidRollKp = 6.0f, pidRollKi = 1.5f, pidRollKd = 2.0f;
    float pidPitchKp = 9.0f, pidPitchKi = 1.0f, pidPitchKd = 5.0f;
    float pidYawKp = 0.15f, pidYawKi = 0.03f, pidYawFF = 1.0f;

    // Asistencias
    float altitudeHoldKp = 1.5f;
    float yawHoldKp = 2.0f;
    float failsafeTimeout = 2.0f;  // s sin empuje antes de aterrizar
    float failsafeDescentPerSecond = 0.25f;
    float trimStepDeg = 0.2f;
};

}  // namespace drone
