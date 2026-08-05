#pragma once

#include "core/math/Vec3.h"

namespace drone {

// Caja de colisión alineada a ejes: centro + extensión completa.
struct Obstacle {
    Vec3 center;
    Vec3 size;
};

}  // namespace drone
