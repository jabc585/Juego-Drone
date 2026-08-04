#pragma once

// Constantes físicas y de juego centralizadas (PLAN2.md R8).
// Fase 2 pendiente (tarea 2.6): sobreescritura desde assets/config/game.toml.
namespace drone::config {

// --- Física ---
inline constexpr float kGravity = 9.81f;          // m/s²
inline constexpr float kDroneMass = 1.2f;         // kg
inline constexpr float kMaxThrust = 25.0f;        // N por eje con input 1.0
inline constexpr float kDragCoefficient = 0.35f;  // N·s/m — arrastre y empuje del viento
inline constexpr float kDroneRadius = 0.4f;       // m — radio de colisión
inline constexpr float kCrashSpeed = 8.0f;        // m/s — impacto que termina la partida

// --- Batería ---
inline constexpr float kBatteryMax = 100.0f;
inline constexpr float kBatteryPerNewton = 0.02f;     // % por N·s de empuje
inline constexpr float kBatteryLowThreshold = 20.0f;  // % — dispara EventType::BatteryLow

// --- Bucle de juego ---
inline constexpr float kFixedTimestep = 1.0f / 60.0f;  // s — ADR-001
inline constexpr float kMaxFrameTime = 0.25f;          // s — clamp anti espiral de la muerte
inline constexpr float kThrustPulseSeconds = 0.35f;    // s — duración del empuje por pulsación

// --- Progresión ---
inline constexpr int kXPPerLevelBase = 100;  // umbral = nivel * base
inline constexpr int kUnlockLevel = 3;
inline constexpr float kXPPerSecond = 5.0f;  // XP por segundo de vuelo

// --- Mundo ---
inline constexpr float kWorldHalfExtent = 100.0f;  // m — límites en ±X/±Z
inline constexpr float kMaxAltitude = 50.0f;       // m

// --- Entorno / viento ---
inline constexpr float kWindBaseSpeed = 1.5f;    // m/s por punto de dificultad
inline constexpr float kWindSmoothing = 2.0f;    // 1/s — suavizado hacia la racha objetivo
inline constexpr float kGustMinInterval = 2.0f;  // s
inline constexpr float kGustMaxInterval = 6.0f;  // s
inline constexpr float kDifficultyRamp = 0.02f;  // +dificultad por segundo
inline constexpr float kMaxDifficulty = 10.0f;

}  // namespace drone::config
