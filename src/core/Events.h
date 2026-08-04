#pragma once

namespace drone {

enum class EventType {
    BatteryLow,     // value = % restante
    BatteryEmpty,   // value = 0
    Collision,      // value = velocidad de impacto en m/s
    LevelUp,        // value = nivel alcanzado
    DroneUnlocked,  // value = nivel de desbloqueo
    GameSaved,      // value sin uso
    GameLoaded,     // value sin uso
};

struct Event {
    EventType type;
    float value = 0.0f;
};

}  // namespace drone
