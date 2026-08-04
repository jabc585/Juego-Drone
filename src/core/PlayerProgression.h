#pragma once

#include <string>

#include "core/EventBus.h"
#include "core/GameConfig.h"

namespace drone {

class PlayerProgression {
public:
    PlayerProgression(const GameConfig& cfg, EventBus& bus) : m_config(cfg), m_bus(bus) {}

    void addExperience(int amount);
    void reset();

    // Restaura una partida guardada sin publicar eventos (evita el spam de
    // LevelUp/DroneUnlocked al cargar). Valores fuera de rango se recortan.
    void restore(int level, int experience);

    int getLevel() const { return m_level; }
    int getExperience() const { return m_experience; }
    int getExperienceToNext() const { return m_config.xpPerLevelBase; }

    std::string unlockDrone() const;

private:
    void levelUp();

    const GameConfig& m_config;
    EventBus& m_bus;
    int m_level = 1;
    int m_experience = 0;
};

}  // namespace drone
