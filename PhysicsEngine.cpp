#include "PhysicsEngine.h"
#include <iostream>

PhysicsEngine::PhysicsEngine() {
    // Inicialización de parámetros físicos
}

void PhysicsEngine::update(float deltaTime) {
    // Actualización de físicas: gravedad, colisiones, resistencia al aire
    std::cout << "Actualizando físicas con deltaTime: " << deltaTime << std::endl;
    // Futuro: integrar librería Bullet Physics para cálculos avanzados
}

