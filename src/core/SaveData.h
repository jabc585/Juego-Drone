#pragma once

#include "core/GameState.h"

namespace drone {

// Datos serializables de una partida guardada. Puro DTO, sin I/O.
// Vive en core para que GameController pueda aplicarlo sin depender de app.
struct SaveData {
    int version = 1;
    float dronePosX = 0, dronePosY = 0, dronePosZ = 0;
    float droneVelX = 0, droneVelY = 0, droneVelZ = 0;
    float battery = 100.0f;
    int level = 1;
    int experience = 0;
    float difficulty = 1.0f;
    float simTime = 0.0f;
    GameState state = GameState::Playing;
};

}  // namespace drone
