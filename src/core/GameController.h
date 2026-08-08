#pragma once

#include "core/Commands.h"
#include "core/GameConfig.h"
#include "core/GameState.h"
#include "core/PlayerProgression.h"
#include "core/SaveData.h"
#include "core/World.h"
#include "frontend/IInputSource.h"
#include "frontend/IRenderer.h"
#include "physics/PhysicsSettings.h"

namespace drone {

class GameController {
public:
    GameController(IInputSource& input, IRenderer& renderer, const GameConfig& cfg,
                   const physics::PhysicsSettings& physCfg);

    void run();
    void tick(float frameSeconds);
    GameState state() const { return m_state; }
    EventBus& getEventBus() { return m_world.events(); }
    WorldState getSnapshot() const { return makeState(); }

    bool saveRequested() const { return m_saveRequested; }
    void clearSaveRequest() { m_saveRequested = false; }
    bool loadRequested() const { return m_loadRequested; }
    void clearLoadRequest() { m_loadRequested = false; }

    void applyLoad(const struct SaveData& data);

private:
    // Que esta pidiendo la barra espaciadora en este momento.
    enum class ClimbMode {
        Off,         // nadie sube: manda la gravedad
        Step,        // una pulsacion: subir un metro y soltar
        Continuous,  // dos pulsaciones: subir hasta que se corte
    };

    void handleCommand(Command cmd);
    void onElevate();
    float climbThrottle();
    void fixedUpdate(float dt);
    void restart();
    WorldState makeState() const;

    const GameConfig& m_config;
    IInputSource& m_input;
    IRenderer& m_renderer;
    World m_world;
    PlayerProgression m_progression;

    GameState m_state = GameState::Booting;
    float m_accumulator = 0.0f;
    float m_xpFraction = 0.0f;
    bool m_crashed = false;

    float m_pulseDir[3] = {0.0f, 0.0f, 0.0f};
    float m_pulseTime[3] = {0.0f, 0.0f, 0.0f};

    ClimbMode m_climb = ClimbMode::Off;
    float m_climbTargetY = 0.0f;
    float m_climbUntil = 0.0f;
    float m_gravity = 9.81f;
    // Reloj de entrada, solo para medir el hueco entre pulsaciones. Arranca
    // muy atras para que la primera nunca cuente como doble.
    float m_inputClock = 0.0f;
    float m_lastElevate = -1000.0f;

    bool m_saveRequested = false;
    bool m_loadRequested = false;
};

}  // namespace drone
