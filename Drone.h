#ifndef DRONE_H
#define DRONE_H

#include <iostream>
#include <cmath>

class Drone {
private:
    // Posición y estado físico
    float x, y, z;          // Posición en el espacio 3D
    float velocity;         // Velocidad actual
    float inertia;          // Factor de inercia para simular la inercia del dron
    float battery;          // Nivel de batería
    // Variables para simulación de viento
    float windForceX, windForceY, windForceZ;

public:
    Drone();
    void move(float deltaX, float deltaY, float deltaZ);
    void applyWind(float forceX, float forceY, float forceZ);
    void updatePhysics(float deltaTime);
    void displayStatus() const;
};

#endif

