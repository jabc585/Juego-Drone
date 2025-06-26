#ifndef GAME_H
#define GAME_H

#include "Drone.h"
#include "Environment.h"
#include "PhysicsEngine.h"
#include "UIManager.h"
#include "PlayerProgression.h"

class Game {
private:
    Drone drone;
    Environment environment;
    PhysicsEngine physicsEngine;
    UIManager uiManager;
    PlayerProgression playerProgression;
    bool isRunning;
    bool isPaused;
public:
    Game();
    void init();
    void run();
    void processInput();
    void update(float deltaTime);
    void render();
    void togglePause();
    void shutdown();
};

#endif

