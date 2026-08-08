#include "core/FailsafeTrim.h"

#include <algorithm>

namespace drone {

void Failsafe::reset() {
    active = false;
    timeSinceInput = 0;
    landingThrottle = 0;
}

float Failsafe::compute(float inputThrottle, float hoverThrottle, bool grounded, float dt) {
    if (inputThrottle > 0.01f || grounded) {
        // Posado no hay nada que aterrizar. Antes el failsafe se enganchaba
        // en el suelo y ya no soltaba.
        timeSinceInput = 0;
        active = false;
        return inputThrottle;
    }

    timeSinceInput += dt;
    if (timeSinceInput < timeoutSeconds)
        return inputThrottle;

    if (!active) {
        // El descenso arranca desde el empuje de hover, no desde el último
        // valor visto: si no, empezaba en cero y no había descenso alguno.
        active = true;
        landingThrottle = hoverThrottle;
    }
    landingThrottle = std::max(0.0f, landingThrottle - descentPerSecond * dt);
    return landingThrottle;
}

}  // namespace drone
