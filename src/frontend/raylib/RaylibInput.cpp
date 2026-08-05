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

    // Movimiento: continuo mientras la tecla siga pulsada.
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        m_queue.push_back(Command::ThrustForward);
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        m_queue.push_back(Command::ThrustBackward);
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        m_queue.push_back(Command::StrafeLeft);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        m_queue.push_back(Command::StrafeRight);
    if (IsKeyDown(KEY_Q))
        m_queue.push_back(Command::Ascend);
    if (IsKeyDown(KEY_E))
        m_queue.push_back(Command::Descend);

    // Acciones: una sola vez por pulsación.
    if (IsKeyPressed(KEY_P))
        m_queue.push_back(Command::Pause);
    if (IsKeyPressed(KEY_R))
        m_queue.push_back(Command::Restart);
    if (IsKeyPressed(KEY_F5))
        m_queue.push_back(Command::Save);
    if (IsKeyPressed(KEY_F9))
        m_queue.push_back(Command::Load);
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
