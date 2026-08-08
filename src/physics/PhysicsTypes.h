#pragma once

#include <cstdint>

#include "core/math/Vec3.h"

namespace drone::physics {

struct BodyId {
    uint32_t index = 0;
    uint32_t generation = 0;
    bool valid() const { return generation != 0; }
};

// La generación forma parte de la identidad: comparar solo el índice haría
// que un id caducado pareciese el cuerpo que ocupa ahora esa ranura.
inline bool operator==(BodyId a, BodyId b) {
    return a.index == b.index && a.generation == b.generation;
}
inline bool operator!=(BodyId a, BodyId b) {
    return !(a == b);
}

struct ColliderId {
    uint32_t index = 0;
    uint32_t generation = 0;
};

struct Transform {
    Vec3 position;
    Vec3 scale{1.0f, 1.0f, 1.0f};
    float qx = 0, qy = 0, qz = 0, qw = 1.0f;
};

enum class BodyType { Static, Dynamic, Kinematic };
enum class ShapeType { Box, Sphere, Capsule, ConvexMesh, ConcaveMesh, HeightField };

// Fase de un contacto (grafico.md §6.7). Mapea 1:1 con
// rp3d::CollisionCallback::ContactPair::EventType.
enum class ContactPhase { Enter, Stay, Exit };

struct ContactEvent {
    BodyId a;
    BodyId b;
    Vec3 point;
    Vec3 normal;
    float penetration = 0.0f;
    // ESTIMADO, no publicado por el solver (§3.2 H2): se calcula con la
    // velocidad relativa previa al paso. Ignora fricción, reparto entre
    // puntos de contacto e iteraciones. Vale para decidir si un impacto es
    // fatal o para escalar un sonido; no para nada que exija exactitud.
    float impulse = 0.0f;
    float impactSpeed = 0.0f;  // |v_rel · n| antes del paso, en m/s
    ContactPhase phase = ContactPhase::Enter;
};

struct RaycastHit {
    BodyId body;
    Vec3 point;
    Vec3 normal;
    float distance = 0;
    bool hit = false;
};

}  // namespace drone::physics
