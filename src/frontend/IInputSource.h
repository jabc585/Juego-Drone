#pragma once

#include "core/Commands.h"

namespace drone {

class IInputSource {
public:
    virtual ~IInputSource() = default;

    // NUNCA bloquea: devuelve Command::None si no hay entrada pendiente.
    // Con la entrada agotada de forma permanente (EOF), devuelve Quit.
    virtual Command poll() = 0;
};

}  // namespace drone
