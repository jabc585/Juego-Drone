#include "core/PlayerProgression.h"

#include "core/Config.h"

namespace drone {

int PlayerProgression::getExperienceToNext() const {
    return config::kXPPerLevelBase;
}

void PlayerProgression::addExperience(int amount) {
    if (amount <= 0)
        return;
    m_experience += amount;
    // Multinivel en una sola llamada, conservando el excedente (cierra B7).
    while (m_experience >= getExperienceToNext()) {
        m_experience -= getExperienceToNext();
        levelUp();
    }
}

void PlayerProgression::levelUp() {
    m_level++;
    if (m_bus != nullptr) {
        m_bus->publish({EventType::LevelUp, static_cast<float>(m_level)});
        if (m_level == config::kUnlockLevel) {
            m_bus->publish({EventType::DroneUnlocked, static_cast<float>(m_level)});
        }
    }
}

void PlayerProgression::reset() {
    m_level = 1;
    m_experience = 0;
}

std::string PlayerProgression::unlockDrone() const {
    if (m_level >= config::kUnlockLevel) {
        return "Nuevo dron desbloqueado: Modelo X avanzado.";
    }
    return "Sube de nivel para desbloquear nuevos drones.";
}

}  // namespace drone
