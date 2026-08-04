#include "core/PlayerProgression.h"

#include <algorithm>

namespace drone {

void PlayerProgression::addExperience(int amount) {
    if (amount <= 0)
        return;
    m_experience += amount;
    while (m_experience >= getExperienceToNext()) {
        m_experience -= getExperienceToNext();
        levelUp();
    }
}

void PlayerProgression::levelUp() {
    m_level++;
    m_bus.publish({EventType::LevelUp, static_cast<float>(m_level)});
    if (m_level == m_config.unlockLevel) {
        m_bus.publish({EventType::DroneUnlocked, static_cast<float>(m_level)});
    }
}

void PlayerProgression::reset() {
    m_level = 1;
    m_experience = 0;
}

void PlayerProgression::restore(int level, int experience) {
    m_level = std::max(1, level);
    m_experience = std::max(0, std::min(experience, getExperienceToNext() - 1));
}

std::string PlayerProgression::unlockDrone() const {
    if (m_level >= m_config.unlockLevel) {
        return "Nuevo dron desbloqueado: Modelo X avanzado.";
    }
    return "Sube de nivel para desbloquear nuevos drones.";
}

}  // namespace drone
