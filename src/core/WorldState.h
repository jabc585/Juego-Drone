#pragma once

#include <string>
#include <vector>

#include "core/GameState.h"
#include "core/Obstacle.h"
#include "core/math/Vec3.h"

namespace drone {

// Snapshot de solo lectura que el core entrega al frontend en cada frame.
struct WorldState {
    Vec3 dronePosition;
    Vec3 droneVelocity;
    Vec3 wind;
    float battery = 100.0f;
    float difficulty = 1.0f;
    float simTime = 0.0f;
    int level = 1;
    int experience = 0;
    int experienceToNext = 100;
    GameState state = GameState::Booting;
    std::string environmentName;
    std::vector<Obstacle> obstacles;
    // Orientación como cuaternión (qx,qy,qz,qw). qw=1, resto=0 ⇒ identidad.
    float droneQx = 0, droneQy = 0, droneQz = 0, droneQw = 1.0f;
    // La misma actitud en radianes, que es lo que el HUD puede mostrar:
    // roll > 0 ⇒ lado derecho arriba; pitch > 0 ⇒ morro arriba.
    float droneRoll = 0, dronePitch = 0, droneYaw = 0;
    bool altitudeHoldActive = false;
    bool failsafeActive = false;
    float targetAltitude = 0;
    float physicsMs = 0;
    float groundDistance = 0;
    uint32_t bodiesAwake = 0;
    // Empuje de cada motor como fraccion de su maximo (0..1), en el orden del
    // mezclador. El frontend lo usa para que el brillo de las helices siga al
    // mando en vez de ser un adorno fijo.
    float motorThrust[4] = {0, 0, 0, 0};
    float nearestZoneDist = -1;
    std::vector<Vec3> landingZonePositions;
};

}  // namespace drone
