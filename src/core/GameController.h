#pragma once

#include "core/Commands.h"
#include "core/GameState.h"
#include "core/PlayerProgression.h"
#include "core/World.h"
#include "frontend/IInputSource.h"
#include "frontend/IRenderer.h"

namespace drone {

// Orquestador: máquina de estados (PLAN2.md §6.5) + bucle de timestep fijo
// (ADR-001). Recibe el frontend por inyección; jamás conoce implementaciones
// concretas ni hace I/O.
class GameController {
public:
    GameController(IInputSource& input, IRenderer& renderer);

    // Bloquea hasta que el estado llegue a ShuttingDown.
    void run();

    // Una iteración del bucle con un tiempo de frame dado; expuesto para
    // poder testear el controlador sin reloj real.
    void tick(float frameSeconds);

    GameState state() const { return m_state; }

private:
    void handleCommand(Command cmd);
    void fixedUpdate(float dt);
    void restart();
    WorldState makeState() const;

    IInputSource& m_input;
    IRenderer& m_renderer;
    World m_world;
    PlayerProgression m_progression;

    GameState m_state = GameState::Booting;
    float m_accumulator = 0.0f;
    float m_xpFraction = 0.0f;
    bool m_crashed = false;

    // Empuje por pulsación: cada eje mantiene su dirección durante
    // kThrustPulseSeconds tras la última tecla.
    float m_pulseDir[3] = {0.0f, 0.0f, 0.0f};
    float m_pulseTime[3] = {0.0f, 0.0f, 0.0f};
};

}  // namespace drone
