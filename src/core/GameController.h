#pragma once

#include "core/Commands.h"
#include "core/GameConfig.h"
#include "core/GameState.h"
#include "core/PlayerProgression.h"
#include "core/SaveData.h"
#include "core/World.h"
#include "frontend/IInputSource.h"
#include "frontend/IRenderer.h"

namespace drone {

class GameController {
public:
    GameController(IInputSource& input, IRenderer& renderer, const GameConfig& cfg);

    void run();
    void tick(float frameSeconds);
    GameState state() const { return m_state; }
    EventBus& getEventBus() { return m_world.events(); }
    WorldState getSnapshot() const { return makeState(); }

    // Banderas de guardado/carga que el app layer consulta tras tick().
    bool saveRequested() const { return m_saveRequested; }
    void clearSaveRequest() { m_saveRequested = false; }
    bool loadRequested() const { return m_loadRequested; }
    void clearLoadRequest() { m_loadRequested = false; }

    // Aplica datos de una partida guardada.
    void applyLoad(const struct SaveData& data);

private:
    void handleCommand(Command cmd);
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

    bool m_saveRequested = false;
    bool m_loadRequested = false;
};

}  // namespace drone
