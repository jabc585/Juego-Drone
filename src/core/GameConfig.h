#pragma once

namespace drone {

// Valores de configuracion con defaults de fabrica. Se construye con los
// valores en tiempo de compilacion; main puede sobreescribir desde
// assets/config/game.toml antes de inyectarla en GameController.
// Tests construyen instancias sin tocar ficheros ni I/O.
struct GameConfig {
    // --- Fisica ---
    float gravity = 9.81f;
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
};

}  // namespace drone
