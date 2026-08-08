#pragma once

#include "core/math/Vec3.h"

namespace drone {

// Para qué sirve la caja al dibujarla. La física las trata todas igual: es
// solo para que el frontend no pinte un tronco del color de un rascacielos.
enum class ObstacleKind {
    Building,
    Trunk,
    Canopy,
    Rock,
};

// Caja de colisión alineada a ejes: centro + extensión completa.
struct Obstacle {
    Vec3 center;
    Vec3 size;
    ObstacleKind kind = ObstacleKind::Building;
};

}  // namespace drone
