#include "frontend/raylib/RaylibInput.h"

#include <raylib.h>

namespace drone {

void RaylibInput::collectFrameCommands() {
    m_queue.clear();
    m_next = 0;

    if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
        m_queue.push_back(Command::Quit);
        return;
    }

    // Con la camara libre el movimiento es de la camara, no del dron: raylib
    // lee W/A/S/D dentro de UpdateCamera y sin esto se pilotaban los dos.
    const bool droneHasKeys = !m_view.freeCameraOwnsMovement();

    // Movimiento: continuo mientras la tecla siga pulsada.
    if (droneHasKeys && (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)))
        m_queue.push_back(Command::ThrustForward);
    if (droneHasKeys && (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)))
        m_queue.push_back(Command::ThrustBackward);
    if (droneHasKeys && (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)))
        m_queue.push_back(Command::StrafeLeft);
    if (droneHasKeys && (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)))
        m_queue.push_back(Command::StrafeRight);
    // Acciones: una sola vez por pulsación.
    // El espacio va aquí y no arriba: su efecto depende de cuántas pulsaciones
    // se cuenten, así que mantenerlo apretado no debe generar una por frame.
    if (droneHasKeys && IsKeyPressed(KEY_SPACE))
        m_queue.push_back(Command::Ascend);
    if (IsKeyPressed(KEY_P))
        m_queue.push_back(Command::Pause);
    if (IsKeyPressed(KEY_R))
        m_queue.push_back(Command::Restart);
    if (IsKeyPressed(KEY_F5))
        m_queue.push_back(Command::Save);
    if (IsKeyPressed(KEY_F9))
        m_queue.push_back(Command::Load);
    if (IsKeyPressed(KEY_H))
        m_queue.push_back(Command::AltitudeToggle);
    if (IsKeyPressed(KEY_F1))
        m_queue.push_back(Command::TrimPitchUp);
    if (IsKeyPressed(KEY_F2))
        m_queue.push_back(Command::TrimPitchDown);
    if (IsKeyPressed(KEY_F3))
        m_queue.push_back(Command::TrimRollLeft);
    if (IsKeyPressed(KEY_F4))
        m_queue.push_back(Command::TrimRollRight);
    if (IsKeyPressed(KEY_ONE))
        m_queue.push_back(Command::Option1);
    if (IsKeyPressed(KEY_TWO))
        m_queue.push_back(Command::Option2);
    if (IsKeyPressed(KEY_THREE))
        m_queue.push_back(Command::Option3);
}

Command RaylibInput::poll() {
    if (m_needsRefill) {
        collectFrameCommands();
        m_needsRefill = false;
    }

    if (m_next < m_queue.size())
        return m_queue[m_next++];

    // Cola agotada: el próximo poll() pertenece ya al siguiente frame.
    m_needsRefill = true;
    return Command::None;
}

}  // namespace drone
