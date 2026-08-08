#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "core/GameConfig.h"
#include "core/Obstacle.h"
#include "core/math/Vec3.h"

namespace drone {

class Environment {
public:
    explicit Environment(const GameConfig& cfg);

    void loadEnvironment(const std::string& environmentName);
    void setSeed(uint32_t seed);
    void step(float dt);
    void reset();
    // Restaura el tiempo transcurrido (y con él la dificultad) al cargar
    // una partida guardada.
    void restoreProgress(float elapsed);

    Vec3 wind() const { return m_wind; }
    float difficulty() const { return m_difficulty; }
    const std::string& name() const { return m_name; }
    const std::vector<Obstacle>& obstacles() const { return m_obstacles; }
    // Donde el entorno pone sus plataformas. Las decide el entorno y no
    // World: el generador de obstaculos tiene que dejarlas despejadas.
    const std::vector<Vec3>& landingZones() const { return m_landingZones; }

private:
    void scheduleNextGust();
    std::vector<Vec3> landingZonesFor(const std::string& environmentName) const;

    const GameConfig& m_config;
    std::string m_name;
    std::vector<Obstacle> m_obstacles;
    std::vector<Vec3> m_landingZones;
    std::mt19937 m_rng;
    uint32_t m_seed = 0;
    Vec3 m_wind;
    Vec3 m_windTarget;
    float m_difficulty = 1.0f;
    float m_elapsed = 0.0f;
    float m_timeToNextGust = 0.0f;
};

}  // namespace drone
