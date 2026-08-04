#pragma once

#include <string>

#include "core/EventBus.h"

namespace drone {

// XP y niveles. Publica LevelUp y DroneUnlocked en el bus (PLAN2.md R7).
class PlayerProgression {
public:
    explicit PlayerProgression(EventBus* bus = nullptr) : m_bus(bus) {}

    void addExperience(int amount);
    void reset();

    int getLevel() const { return m_level; }
    int getExperience() const { return m_experience; }
    int getExperienceToNext() const;

    std::string unlockDrone() const;

private:
    void levelUp();

    EventBus* m_bus;
    int m_level = 1;
    int m_experience = 0;
};

}  // namespace drone
