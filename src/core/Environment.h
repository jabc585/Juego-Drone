#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "core/math/Vec3.h"

namespace drone {

// Caja de colisión alineada a ejes: centro + extensión completa.
struct Obstacle {
    Vec3 center;
    Vec3 size;
};

// Entorno con estado real (PLAN2.md R6): viento por rachas suavizadas,
// dificultad que crece con el tiempo y obstáculos AABB.
class Environment {
public:
    Environment();

    void loadEnvironment(const std::string& environmentName);
    // Determinismo para tests: misma semilla ⇒ misma secuencia de rachas.
    void setSeed(uint32_t seed);
    void step(float dt);
    void reset();

    Vec3 wind() const { return m_wind; }
    float difficulty() const { return m_difficulty; }
    const std::string& name() const { return m_name; }
    const std::vector<Obstacle>& obstacles() const { return m_obstacles; }

private:
    void scheduleNextGust();

    std::string m_name;
    std::vector<Obstacle> m_obstacles;
    std::mt19937 m_rng;
    uint32_t m_seed = 0;
    Vec3 m_wind;
    Vec3 m_windTarget;
    float m_difficulty = 1.0f;
    float m_elapsed = 0.0f;
    float m_timeToNextGust = 0.0f;
};

}  // namespace drone
