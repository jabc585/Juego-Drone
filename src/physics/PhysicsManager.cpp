#include "physics/PhysicsManager.h"

#include <reactphysics3d/reactphysics3d.h>

#include <cmath>
#include <set>
#include <unordered_map>

namespace drone::physics {

namespace {

rp3d::Vector3 toRp3d(const Vec3& v) {
    return {v.x, v.y, v.z};
}
Vec3 fromRp3d(const rp3d::Vector3& v) {
    return {v.x, v.y, v.z};
}

}  // namespace

struct BodyRecord {
    rp3d::RigidBody* body = nullptr;
    rp3d::Transform prevTransform;
    rp3d::Vector3 prevLinearVelocity;
    bool isStatic = false;
    float mass = 0.0f;
    float bounciness = 0.0f;
    uint64_t userData = 0;
};

// Puente de contactos (grafico.md §6.7).
//
// Regla dura: rp3d invoca esto DENTRO de world->update(), con el solver en
// curso. Aquí no se crea ni se destruye nada — solo se encola. El despacho
// ocurre después, ya fuera del paso.
class ContactBridge final : public rp3d::EventListener {
public:
    using Lookup = std::unordered_map<const rp3d::Body*, BodyId>;

    // Se guarda el pool, no punteros a sus elementos: el vector interno
    // realoja al crecer y cualquier puntero cacheado quedaría colgando.
    ContactBridge(const Lookup& lookup, const HandlePool<BodyRecord>& pool)
        : m_lookup(lookup), m_pool(pool) {}

    void clear() {
        m_events.clear();
        m_triggerPairs.clear();
    }
    const std::vector<ContactEvent>& events() const { return m_events; }

    void onContact(const rp3d::CollisionCallback::CallbackData& data) override {
        for (rp3d::uint32 p = 0; p < data.getNbContactPairs(); ++p) {
            const auto pair = data.getContactPair(p);
            const BodyId a = idOf(pair.getBody1());
            const BodyId b = idOf(pair.getBody2());
            if (!a.valid() || !b.valid())
                continue;

            ContactEvent ev;
            ev.a = a;
            ev.b = b;
            ev.phase = phaseOf(pair.getEventType());

            // En un ContactExit rp3d ya no reporta puntos: el par se ha
            // perdido. Se emite igualmente para que quien lleve el mapa de
            // contactos activos pueda borrar la entrada.
            if (pair.getNbContactPoints() > 0) {
                const auto cp = pair.getContactPoint(0);
                ev.normal = fromRp3d(cp.getWorldNormal());
                ev.penetration = cp.getPenetrationDepth();
                const rp3d::Transform t = pair.getCollider1()->getLocalToWorldTransform();
                ev.point = fromRp3d(t * cp.getLocalPointOnCollider1());
                estimate(ev, a, b);
            }
            m_events.push_back(ev);
        }
    }

    void onTrigger(const rp3d::OverlapCallback::CallbackData& data) override {
        // Los nombres son getNbOverlappingPairs/getOverlappingPair: la
        // variante sin "ping" no existe en rp3d 0.10.2 y no compilaba.
        for (rp3d::uint32 p = 0; p < data.getNbOverlappingPairs(); ++p) {
            const auto pair = data.getOverlappingPair(p);
            BodyId a = idOf(pair.getBody1());
            BodyId b = idOf(pair.getBody2());
            if (!a.valid() || !b.valid())
                continue;
            m_triggerPairs.insert({a.index, b.index});
            m_triggerPairs.insert({b.index, a.index});
        }
    }

    bool isTriggerOverlapping(BodyId a, BodyId b) const {
        return m_triggerPairs.count({a.index, b.index}) > 0;
    }

    void clearTriggers() { m_triggerPairs.clear(); }

private:
    static ContactPhase phaseOf(rp3d::CollisionCallback::ContactPair::EventType t) {
        switch (t) {
            case rp3d::CollisionCallback::ContactPair::EventType::ContactStart:
                return ContactPhase::Enter;
            case rp3d::CollisionCallback::ContactPair::EventType::ContactExit:
                return ContactPhase::Exit;
            case rp3d::CollisionCallback::ContactPair::EventType::ContactStay:
                break;
        }
        return ContactPhase::Stay;
    }

    BodyId idOf(const rp3d::Body* body) const {
        const auto it = m_lookup.find(body);
        return it == m_lookup.end() ? BodyId{} : it->second;
    }

    const BodyRecord* recordOf(BodyId id) const { return m_pool.get({id.index, id.generation}); }

    // Impulso estimado (H2): el solver no lo publica, así que se calcula con
    // la velocidad relativa ANTES del paso, que captureTransforms guardó.
    //   j = (1 + e) · |v_rel · n| · m_ef,  m_ef = mA·mB/(mA+mB); mA si el
    //   otro es estático.
    void estimate(ContactEvent& ev, BodyId a, BodyId b) const {
        const BodyRecord* ra = recordOf(a);
        const BodyRecord* rb = recordOf(b);
        if (ra == nullptr || rb == nullptr)
            return;

        const rp3d::Vector3 n = toRp3d(ev.normal);
        const rp3d::Vector3 vRel = ra->prevLinearVelocity - rb->prevLinearVelocity;
        ev.impactSpeed = std::fabs(vRel.dot(n));

        float mEff;
        if (rb->isStatic || rb->mass <= 0.0f)
            mEff = ra->mass;
        else if (ra->isStatic || ra->mass <= 0.0f)
            mEff = rb->mass;
        else
            mEff = (ra->mass * rb->mass) / (ra->mass + rb->mass);

        const float e = std::fmax(ra->bounciness, rb->bounciness);
        ev.impulse = (1.0f + e) * ev.impactSpeed * mEff;
    }

    const Lookup& m_lookup;
    const HandlePool<BodyRecord>& m_pool;
    std::vector<ContactEvent> m_events;
    std::set<std::pair<uint32_t, uint32_t>> m_triggerPairs;
};

struct PhysicsManager::Impl {
    PhysicsSettings settings;
    rp3d::PhysicsCommon common;
    rp3d::PhysicsWorld* world = nullptr;
    HandlePool<BodyRecord> bodies;

    // rp3d entrega punteros a Body en los callbacks; esto los devuelve al
    // dominio de handles sin exponer punteros al exterior.
    std::unordered_map<const rp3d::Body*, BodyId> byBody;

    std::unique_ptr<ContactBridge> bridge;

    HandlePool<BodyRecord>::Handle handle(BodyId id) const { return {id.index, id.generation}; }

    BodyRecord* record(BodyId id) { return bodies.get(handle(id)); }
    const BodyRecord* record(BodyId id) const { return bodies.get(handle(id)); }
};

PhysicsManager::PhysicsManager(const PhysicsSettings& s) : m_impl(std::make_unique<Impl>()) {
    m_impl->settings = s;

    rp3d::PhysicsWorld::WorldSettings ws;
    ws.gravity = toRp3d(s.gravity);
    ws.persistentContactDistanceThreshold = s.persistentContactDistance;
    ws.defaultFrictionCoefficient = s.defaultFrictionCoefficient;
    ws.defaultBounciness = s.defaultBounciness;
    ws.restitutionVelocityThreshold = s.restitutionVelocityThreshold;
    ws.defaultVelocitySolverNbIterations = static_cast<rp3d::uint16>(s.velocitySolverIterations);
    ws.defaultPositionSolverNbIterations = static_cast<rp3d::uint16>(s.positionSolverIterations);
    ws.cosAngleSimilarContactManifold = s.cosAngleSimilarContactManifold;
    ws.isSleepingEnabled = s.sleepingEnabled;
    ws.defaultTimeBeforeSleep = s.timeBeforeSleep;
    ws.defaultSleepLinearVelocity = s.sleepLinearVelocity;
    ws.defaultSleepAngularVelocity = s.sleepAngularVelocity;

    m_impl->world = m_impl->common.createPhysicsWorld(ws);

    m_impl->bridge = std::make_unique<ContactBridge>(m_impl->byBody, m_impl->bodies);
    m_impl->world->setEventListener(m_impl->bridge.get());
}

PhysicsManager::~PhysicsManager() {
    // El listener sobrevive al mundo por poco: se desengancha primero para
    // que la destrucción de cuerpos no reentre en un puente medio muerto.
    if (m_impl->world != nullptr) {
        m_impl->world->setEventListener(nullptr);
        m_impl->common.destroyPhysicsWorld(m_impl->world);
    }
}

BodyId PhysicsManager::createBody(const Vec3& pos, BodyType type, float mass) {
    const rp3d::Transform transform(toRp3d(pos), rp3d::Quaternion::identity());

    auto* body = m_impl->world->createRigidBody(transform);
    switch (type) {
        case BodyType::Static:
            body->setType(rp3d::BodyType::STATIC);
            break;
        case BodyType::Kinematic:
            body->setType(rp3d::BodyType::KINEMATIC);
            break;
        case BodyType::Dynamic:
            body->setType(rp3d::BodyType::DYNAMIC);
            break;
    }
    if (mass > 0.0f && type == BodyType::Dynamic)
        body->setMass(mass);

    BodyRecord rec;
    rec.body = body;
    rec.isStatic = (type == BodyType::Static);
    rec.mass = body->getMass();
    rec.prevTransform = transform;

    const auto handle = m_impl->bodies.create(std::move(rec));
    const BodyId id{handle.index, handle.generation};

    m_impl->byBody[body] = id;
    return id;
}

namespace {

// Densidad = masa/volumen: rp3d deriva masa e inercia de los colliders, así
// que fijar la densidad es lo que hace que updateMassPropertiesFromColliders
// reproduzca la masa pedida en vez de sobrescribirla.
void applyDensity(rp3d::Collider* collider, rp3d::RigidBody* body, float mass, float volume,
                  BodyType type) {
    if (mass <= 0.0f || type != BodyType::Dynamic || volume <= 0.0f)
        return;
    collider->getMaterial().setMassDensity(mass / volume);
    body->updateMassPropertiesFromColliders();
}

}  // namespace

BodyId PhysicsManager::createBoxBody(const Vec3& half, const Vec3& pos, BodyType type, float mass) {
    const BodyId id = createBody(pos, type, mass);
    BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return id;

    auto* shape = m_impl->common.createBoxShape(toRp3d(half));
    auto* collider = rec->body->addCollider(shape, rp3d::Transform::identity());
    applyDensity(collider, rec->body, mass, 8.0f * half.x * half.y * half.z, type);

    rec->mass = rec->body->getMass();
    rec->bounciness = collider->getMaterial().getBounciness();
    return id;
}

BodyId PhysicsManager::createSphereBody(float radius, const Vec3& pos, BodyType type, float mass) {
    const BodyId id = createBody(pos, type, mass);
    BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return id;

    auto* shape = m_impl->common.createSphereShape(radius);
    auto* collider = rec->body->addCollider(shape, rp3d::Transform::identity());
    const float volume = 4.0f / 3.0f * 3.14159265f * radius * radius * radius;
    applyDensity(collider, rec->body, mass, volume, type);

    rec->mass = rec->body->getMass();
    rec->bounciness = collider->getMaterial().getBounciness();
    return id;
}

void PhysicsManager::destroyBody(BodyId id) {
    BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return;
    m_impl->byBody.erase(rec->body);
    m_impl->world->destroyRigidBody(rec->body);
    m_impl->bodies.destroy(m_impl->handle(id));
}

bool PhysicsManager::isValid(BodyId id) const {
    return m_impl->record(id) != nullptr;
}

void PhysicsManager::setFriction(BodyId id, float friction) {
    if (BodyRecord* rec = m_impl->record(id)) {
        auto* collider = rec->body->getCollider(0);
        if (collider)
            collider->getMaterial().setFrictionCoefficient(friction);
    }
}

void PhysicsManager::setBounciness(BodyId id, float bounciness) {
    if (BodyRecord* rec = m_impl->record(id)) {
        auto* collider = rec->body->getCollider(0);
        if (collider)
            collider->getMaterial().setBounciness(bounciness);
    }
}

void PhysicsManager::setMass(BodyId id, float mass) {
    if (BodyRecord* rec = m_impl->record(id)) {
        rec->body->setMass(mass);
    }
}

void PhysicsManager::setLinearDamping(BodyId id, float damping) {
    if (BodyRecord* rec = m_impl->record(id)) {
        rec->body->setLinearDamping(damping);
    }
}

void PhysicsManager::setAngularDamping(BodyId id, float damping) {
    if (BodyRecord* rec = m_impl->record(id)) {
        rec->body->setAngularDamping(damping);
    }
}

void PhysicsManager::setTrigger(BodyId id, bool isTrigger) {
    if (BodyRecord* rec = m_impl->record(id)) {
        auto* collider = rec->body->getCollider(0);
        if (collider)
            collider->setIsTrigger(isTrigger);
    }
}

void PhysicsManager::applyForce(BodyId id, const Vec3& force) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->applyWorldForceAtCenterOfMass(toRp3d(force));
}

void PhysicsManager::applyForceAtPosition(BodyId id, const Vec3& force, const Vec3& point) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->applyWorldForceAtWorldPosition(toRp3d(force), toRp3d(point));
}

void PhysicsManager::applyLocalForceAtLocalPosition(BodyId id, const Vec3& localForce,
                                                    const Vec3& localPoint) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->applyLocalForceAtLocalPosition(toRp3d(localForce), toRp3d(localPoint));
}

void PhysicsManager::applyTorque(BodyId id, const Vec3& torque) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->applyWorldTorque(toRp3d(torque));
}

void PhysicsManager::applyLocalTorque(BodyId id, const Vec3& torque) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->applyLocalTorque(toRp3d(torque));
}

void PhysicsManager::setInertiaTensor(BodyId id, const Vec3& diagonal) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->setLocalInertiaTensor(toRp3d(diagonal));
}

void PhysicsManager::setLinearVelocity(BodyId id, const Vec3& vel) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->setLinearVelocity(toRp3d(vel));
}

void PhysicsManager::setAngularFactor(BodyId id, const Vec3& factor) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->setAngularLockAxisFactor(toRp3d(factor));
}

void PhysicsManager::setActive(BodyId id, bool active) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->body->setIsActive(active);
}

void PhysicsManager::setPosition(BodyId id, const Vec3& position) {
    BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return;
    const rp3d::Transform t(toRp3d(position), rec->body->getTransform().getOrientation());
    rec->body->setTransform(t);
    // El transform previo va con él: si no, el frame siguiente interpola
    // entre el sitio viejo y el nuevo y se ve un salto de un extremo a otro.
    rec->prevTransform = t;
}

void PhysicsManager::setOrientation(BodyId id, float qx, float qy, float qz, float qw) {
    BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return;
    rp3d::Quaternion q(qx, qy, qz, qw);
    rp3d::Transform t(rec->body->getTransform().getPosition(), q);
    rec->body->setTransform(t);
    rec->prevTransform = t;
}

void PhysicsManager::resetBody(BodyId id, const Vec3& position) {
    BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return;
    setPosition(id, position);
    rec->body->setLinearVelocity(rp3d::Vector3::zero());
    rec->body->setAngularVelocity(rp3d::Vector3::zero());
    rec->body->resetForce();
    rec->body->resetTorque();
    rec->prevLinearVelocity = rp3d::Vector3::zero();
    // Un cuerpo dormido ignora la reposición hasta que algo lo toca.
    rec->body->setIsSleeping(false);
}

Transform PhysicsManager::getTransform(BodyId id) const {
    Transform t;
    const BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return t;
    const rp3d::Transform& rp = rec->body->getTransform();
    t.position = fromRp3d(rp.getPosition());
    t.qx = rp.getOrientation().x;
    t.qy = rp.getOrientation().y;
    t.qz = rp.getOrientation().z;
    t.qw = rp.getOrientation().w;
    return t;
}

Vec3 PhysicsManager::getLinearVelocity(BodyId id) const {
    const BodyRecord* rec = m_impl->record(id);
    return rec == nullptr ? Vec3{} : fromRp3d(rec->body->getLinearVelocity());
}

Vec3 PhysicsManager::getAngularVelocity(BodyId id) const {
    const BodyRecord* rec = m_impl->record(id);
    return rec == nullptr ? Vec3{} : fromRp3d(rec->body->getAngularVelocity());
}

float PhysicsManager::getMass(BodyId id) const {
    const BodyRecord* rec = m_impl->record(id);
    return rec == nullptr ? 0.0f : rec->body->getMass();
}

bool PhysicsManager::isSleeping(BodyId id) const {
    const BodyRecord* rec = m_impl->record(id);
    return rec != nullptr && rec->body->isSleeping();
}

uint64_t PhysicsManager::userData(BodyId id) const {
    const BodyRecord* rec = m_impl->record(id);
    return rec == nullptr ? 0 : rec->userData;
}

void PhysicsManager::setUserData(BodyId id, uint64_t data) {
    if (BodyRecord* rec = m_impl->record(id))
        rec->userData = data;
}

Transform PhysicsManager::interpolated(BodyId id, float alpha) const {
    Transform t;
    const BodyRecord* rec = m_impl->record(id);
    if (rec == nullptr)
        return t;
    if (rec->body->isSleeping())
        return getTransform(id);

    const rp3d::Transform interp = rp3d::Transform::interpolateTransforms(
        rec->prevTransform, rec->body->getTransform(), alpha);
    t.position = fromRp3d(interp.getPosition());
    t.qx = interp.getOrientation().x;
    t.qy = interp.getOrientation().y;
    t.qz = interp.getOrientation().z;
    t.qw = interp.getOrientation().w;
    return t;
}

void PhysicsManager::step(float dt) {
    // captureTransforms (§6.4): el transform previo alimenta la
    // interpolación y la velocidad previa, la estimación de impulso. Se
    // guardan ANTES del paso porque después el solver ya ha resuelto el
    // choque y la velocidad de impacto real se ha perdido.
    m_impl->bodies.forEach([](HandlePool<BodyRecord>::Handle, BodyRecord& rec) {
        if (rec.isStatic || rec.body->isSleeping())
            return;
        rec.prevTransform = rec.body->getTransform();
        rec.prevLinearVelocity = rec.body->getLinearVelocity();
    });

    m_impl->bridge->clear();

    m_profiler.beginStep();
    m_impl->world->update(dt);
    m_profiler.endStep(bodyCount(), awakeCount(),
                       static_cast<uint32_t>(m_impl->bridge->events().size()), 1);
}

const std::vector<ContactEvent>& PhysicsManager::contacts() const {
    return m_impl->bridge->events();
}

namespace {

// rp3d::Ray es un SEGMENTO entre dos puntos, no origen + dirección: hay que
// construir el extremo a mano. Pasar la dirección como segundo punto lanzaba
// el rayo hacia una coordenada absoluta del mundo, casi siempre al lado
// contrario, y el altímetro no acertaba nunca.
rp3d::Ray segmentFrom(const Vec3& origin, const Vec3& direction, float maxDist) {
    const float len = std::sqrt(direction.x * direction.x + direction.y * direction.y +
                                direction.z * direction.z);
    const float scale = len > 1e-6f ? maxDist / len : 0.0f;
    const rp3d::Vector3 p1 = toRp3d(origin);
    const rp3d::Vector3 p2(origin.x + direction.x * scale, origin.y + direction.y * scale,
                           origin.z + direction.z * scale);
    return rp3d::Ray(p1, p2);
}

}  // namespace

RaycastHit PhysicsManager::raycastClosest(const Vec3& origin, const Vec3& direction,
                                          float maxDist) const {
    RaycastHit result;
    const rp3d::Ray ray = segmentFrom(origin, direction, maxDist);

    struct ClosestCB : rp3d::RaycastCallback {
        RaycastHit& h;
        float length;
        ClosestCB(RaycastHit& hit, float len) : h(hit), length(len) {}
        rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override {
            // hitFraction es relativo a la longitud del segmento, que aquí ya
            // es maxDist; antes se multiplicaba por un valor sin relación.
            const float d = info.hitFraction * length;
            if (!h.hit || d < h.distance) {
                h.hit = true;
                h.distance = d;
                h.point = fromRp3d(info.worldPoint);
                h.normal = fromRp3d(info.worldNormal);
            }
            // Devolver la fracción acorta el rayo al impacto más cercano.
            return info.hitFraction;
        }
    };

    ClosestCB cb(result, maxDist);
    // El tercer parámetro es una MÁSCARA DE CATEGORÍAS, no una distancia:
    // pasarle los metros filtraba los cuerpos por bits arbitrarios.
    m_impl->world->raycast(ray, &cb);
    return result;
}

bool PhysicsManager::raycastAny(const Vec3& origin, const Vec3& direction, float maxDist) const {
    bool hit = false;
    const rp3d::Ray ray = segmentFrom(origin, direction, maxDist);

    struct AnyCB : rp3d::RaycastCallback {
        bool& h;
        explicit AnyCB(bool& hit) : h(hit) {}
        rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo&) override {
            h = true;
            return 0;  // 0 detiene la consulta: basta con saber que hay algo
        }
    };

    AnyCB cb(hit);
    m_impl->world->raycast(ray, &cb);
    return hit;
}

bool PhysicsManager::triggerOverlap(BodyId a, BodyId b) const {
    return m_impl->bridge->isTriggerOverlapping(a, b);
}

uint32_t PhysicsManager::bodyCount() const {
    return static_cast<uint32_t>(m_impl->bodies.aliveCount());
}

uint32_t PhysicsManager::awakeCount() const {
    uint32_t awake = 0;
    m_impl->bodies.forEach([&awake](HandlePool<BodyRecord>::Handle, const BodyRecord& rec) {
        if (!rec.isStatic && !rec.body->isSleeping())
            ++awake;
    });
    return awake;
}

}  // namespace drone::physics
