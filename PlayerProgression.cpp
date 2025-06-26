#include "PlayerProgression.h"
#include <iostream>

PlayerProgression::PlayerProgression() : level(1), experience(0) {}

void PlayerProgression::addExperience(int amount) {
    experience += amount;
    std::cout << "Experiencia añadida: " << amount << ". Total: " << experience << std::endl;
    if (experience >= level * 100) { // Umbral de experiencia para subir de nivel
        levelUp();
    }
}

void PlayerProgression::levelUp() {
    level++;
    experience = 0;
    std::cout << "¡Nivel aumentado a " << level << "!" << std::endl;
}

void PlayerProgression::displayProgress() const {
    std::cout << "Nivel: " << level << " | Experiencia: " << experience << std::endl;
}

std::string PlayerProgression::unlockDrone() {
    // Lógica para desbloquear un nuevo dron según el nivel o logros
    if (level >= 3) {
        return "Nuevo dron desbloqueado: Modelo X avanzado.";
    }
    return "Sube de nivel para desbloquear nuevos drones.";
}

