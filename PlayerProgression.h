#ifndef PLAYERPROGRESSION_H
#define PLAYERPROGRESSION_H

#include <string>

class PlayerProgression {
private:
    int level;
    int experience;
public:
    PlayerProgression();
    void addExperience(int amount);
    void levelUp();
    void displayProgress() const;
    // Método para desbloquear nuevos drones
    std::string unlockDrone();
};

#endif

