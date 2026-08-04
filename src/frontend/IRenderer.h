#pragma once

#include "core/Events.h"
#include "core/WorldState.h"

namespace drone {

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // alpha ∈ [0,1): fracción de step pendiente, para interpolar el render.
    virtual void draw(const WorldState& state, float alpha) = 0;

    // Avisos del core (nivel, batería, colisión...) para mostrar al jugador.
    virtual void onEvent(const Event& event) = 0;
};

}  // namespace drone
