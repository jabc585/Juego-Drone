#pragma once

#include <memory>
#include <vector>

#include "core/math/Vec3.h"
#include "physics/HandlePool.h"
#include "physics/PhysicsProfiler.h"
#include "physics/PhysicsSettings.h"
#include "physics/PhysicsTypes.h"

namespace drone::physics {

// Fachada sobre ReactPhysics3D (grafico.md §6.3, ADR-009/ADR-011).
//
// La API pública no expone ni un solo tipo de rp3d: todo pasa por BodyId,
// Vec3 y Transform. Los headers de rp3d viven dentro del pimpl, y el guard
// de CI comprueba que no se filtren al resto del árbol.
class PhysicsManager {
public:
    explicit PhysicsManager(const PhysicsSettings&);
    ~PhysicsManager();

    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;

    BodyId createBody(const Vec3& position, BodyType type, float mass = 0);
    BodyId createBoxBody(const Vec3& halfExtents, const Vec3& position, BodyType type,
                         float mass = 0);
    BodyId createSphereBody(float radius, const Vec3& position, BodyType type, float mass = 0);
    void destroyBody(BodyId);
    bool isValid(BodyId) const;

    // Materiales (§6.6): friccion y restitucion por collider
    void setFriction(BodyId, float friction);
    void setBounciness(BodyId, float bounciness);
    void setMass(BodyId, float mass);
    void setLinearDamping(BodyId, float damping);
    void setAngularDamping(BodyId, float damping);
    void setTrigger(BodyId, bool isTrigger);

    void applyForce(BodyId, const Vec3& force);
    void applyForceAtPosition(BodyId, const Vec3& force, const Vec3& worldPoint);

    // Fuerza y punto EN EL MARCO DEL CUERPO. Es la primitiva correcta para
    // un motor de dron: el empuje sigue la inclinación del chasis y el punto
    // de aplicación es un desplazamiento fijo respecto al centro de masa.
    // Usar la variante de mundo con un desplazamiento local aplica la fuerza
    // en un punto absoluto del mapa y el par depende de dónde esté el dron.
    void applyLocalForceAtLocalPosition(BodyId, const Vec3& localForce, const Vec3& localPoint);
    void applyTorque(BodyId, const Vec3& torque);
    void applyLocalTorque(BodyId, const Vec3& torque);

    // El tensor de inercia de la esfera de colisión es ~13x el de un quad
    // real: sin corregirlo el dron responde en actitud diez veces más lento
    // de lo que debería.
    void setInertiaTensor(BodyId, const Vec3& diagonal);
    void setLinearVelocity(BodyId, const Vec3& vel);
    void setAngularFactor(BodyId, const Vec3& factor);
    void setActive(BodyId, bool active);

    // Teletransporte: reposiciona el cuerpo en rp3d y descarta velocidades y
    // fuerzas acumuladas. Sin esto, mover solo el estado de juego no tiene
    // ningún efecto — el siguiente paso lo sobrescribe desde el motor.
    void setPosition(BodyId, const Vec3& position);
    void resetBody(BodyId, const Vec3& position);
    void setOrientation(BodyId, float qx, float qy, float qz, float qw);

    Transform getTransform(BodyId) const;
    Vec3 getLinearVelocity(BodyId) const;
    Vec3 getAngularVelocity(BodyId) const;
    float getMass(BodyId) const;
    bool isSleeping(BodyId) const;
    uint64_t userData(BodyId) const;
    void setUserData(BodyId, uint64_t);

    Transform interpolated(BodyId, float alpha) const;
    void step(float dt);

    // Contactos del último step (§6.7). Los callbacks de rp3d se invocan
    // DENTRO de update(), donde crear o destruir cuerpos es ilegal: el
    // puente solo encola y aquí se despacha, ya fuera del solver.
    const std::vector<ContactEvent>& contacts() const;

    uint32_t bodyCount() const;
    uint32_t awakeCount() const;

    const PhysicsProfiler& profiler() const { return m_profiler; }

    RaycastHit raycastClosest(const Vec3& origin, const Vec3& direction, float maxDist) const;
    bool raycastAny(const Vec3& origin, const Vec3& direction, float maxDist) const;
    bool triggerOverlap(BodyId a, BodyId b) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    PhysicsProfiler m_profiler;
};

}  // namespace drone::physics
