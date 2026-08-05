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
    // Geometría del nivel: el frontend dibuja exactamente lo que colisiona.
    std::vector<Obstacle> obstacles;
};

}  // namespace drone
