#include "Environment.h"
#include <iostream>

Environment::Environment() {
    // Inicialización del entorno
}

void Environment::loadEnvironment(const std::string& environmentName) {
    // Aquí cargarías modelos, texturas y elementos según el entorno
    std::cout << "Cargando entorno: " << environmentName << std::endl;
}

void Environment::render() {
    // Renderización del entorno usando una librería gráfica en el futuro
    std::cout << "Renderizando entorno..." << std::endl;
}

void Environment::increaseDifficulty() {
    // Aquí se podrían aumentar obstáculos, introducir más viento o enemigos
    std::cout << "Aumentando la dificultad del entorno..." << std::endl;
}

