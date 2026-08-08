#pragma once

#include "core/GameState.h"

namespace drone {

struct SaveData {
    int version = 2;
    float dronePosX = 0, dronePosY = 0, dronePosZ = 0;
    float droneVelX = 0, droneVelY = 0, droneVelZ = 0;
    float droneQx = 0, droneQy = 0, droneQz = 0, droneQw = 1.0f;
    float battery = 100.0f;
    int level = 1;
    int experience = 0;
    float difficulty = 1.0f;
    float simTime = 0.0f;
    GameState state = GameState::Playing;
};

}  // namespace drone
