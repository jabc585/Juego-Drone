#include "Drone.h"

Drone::Drone() 
    : x(0), y(0), z(0), velocity(0), inertia(0.9f), battery(100),
      windForceX(0), windForceY(0), windForceZ(0) {}

void Drone::move(float deltaX, float deltaY, float deltaZ) {
    if (battery > 0) {
        // La inercia se aplica para suavizar los cambios de dirección
        x += deltaX * inertia;
        y += deltaY * inertia;
        z += deltaZ * inertia;
        battery -= 0.1f; // Consumo de batería por movimiento
    } else {
        std::cout << "Batería baja. Recarga para continuar." << std::endl;
    }
}

void Drone::applyWind(float forceX, float forceY, float forceZ) {
    // Se simula la influencia del viento
    windForceX = forceX;
    windForceY = forceY;
    windForceZ = forceZ;
}

void Drone::updatePhysics(float deltaTime) {
    // Aplicar el efecto del viento sobre la posición del dron
    x += windForceX * deltaTime;
    y += windForceY * deltaTime;
    z += windForceZ * deltaTime;
    // Ejemplo: aplicar gravedad si no se está ascendiendo activamente
    if (y > 0) {
        y -= 9.81f * deltaTime;
    }
}

void Drone::displayStatus() const {
    std::cout << "Posición: (" << x << ", " << y << ", " << z << ")" << std::endl;
    std::cout << "Batería: " << battery << "%" << std::endl;
}

