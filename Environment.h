#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>

class Environment {
public:
    Environment();
    void loadEnvironment(const std::string& environmentName);
    void render();
    // Posible método para incrementar la dificultad del nivel
    void increaseDifficulty();
};

#endif

