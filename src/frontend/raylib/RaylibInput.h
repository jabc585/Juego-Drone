#pragma once

#include <cstddef>
#include <vector>

#include "frontend/IInputSource.h"

namespace drone {

// Entrada de teclado sobre raylib.
//
// GameController llama a poll() en bucle hasta recibir None, así que el estado
// del teclado se lee UNA sola vez por frame y se sirve desde una cola: leerlo
// en cada llamada haría que una pulsación de P alternase la pausa 32 veces por
// frame y que F5 guardase 32 veces. La cola se recarga en la primera llamada
// posterior a haberse agotado, que siempre cae tras el EndDrawing del frame
// anterior (donde raylib actualiza el estado del teclado).
class RaylibInput : public IInputSource {
public:
    Command poll() override;

private:
    void collectFrameCommands();

    std::vector<Command> m_queue;
    std::size_t m_next = 0;
    bool m_needsRefill = true;
};

}  // namespace drone
