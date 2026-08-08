#pragma once

#include <string>
#include <vector>

#include "core/AltitudeYawHold.h"
#include "core/Drone.h"
#include "core/DronePID.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/FailsafeTrim.h"
#include "core/GameConfig.h"
#include "core/WorldState.h"
#include "core/XFrameMixer.h"
#include "physics/PhysicsManager.h"
#include "physics/PhysicsSettings.h"
#include "physics/PhysicsTypes.h"

namespace drone {

class World {
public:
    World(const GameConfig& gameCfg, const physics::PhysicsSettings& physCfg);

    void step(float dt);
    void reset();
    void loadEnvironment(const std::string& name);

    // Restaura el reloj de simulación (y la dificultad derivada) al cargar.
    void restoreSimTime(float simTime);

    void setThrustInput(const Vec3& input) { m_drone.setThrustInput(input); }

    // Único camino para mover el dron desde fuera de la física. Escribir en
    // Drone::setPosition no sirve: el cuerpo de rp3d es la fuente de verdad
    // y el siguiente paso sobrescribiría el cambio.
    void teleportDrone(const Vec3& position, const Vec3& velocity = {});
    void setDroneOrientation(float qx, float qy, float qz, float qw);

    const Drone& drone() const { return m_drone; }
    Drone& drone() { return m_drone; }
    Environment& environment() { return m_environment; }
    const Environment& environment() const { return m_environment; }
    EventBus& events() { return m_bus; }
    AltitudeHold& altitudeHold() { return m_altHold; }
    YawHold& yawHold() { return m_yawHold; }
    AttitudeTrim& trim() { return m_trim; }
    float simTime() const { return m_simTime; }

    WorldState snapshot() const;

private:
    void syncDroneToPhysics();
    void syncDroneFromPhysics();
    void updateAttitude();
    void updateRightingState();
    void applyRightingAssist();
    // Vector "arriba" del dron en coordenadas de mundo. up.y < 0 ⇒ volcado.
    Vec3 bodyUp() const;
    void applyControlConfig();
    void publishContacts();
    void applyWorldBounds();
    void publishBatteryEvents();
    void createObstacles();
    void createLandingZones();
    void checkLandingZones();
    Vec3 spawnPosition() const;

    const GameConfig& m_gameCfg;
    physics::PhysicsManager m_physics;
    EventBus m_bus;
    Drone m_drone;
    Environment m_environment;
    float m_simTime = 0.0f;
    XFrameMixer m_mixer;
    DronePID m_pid;
    AltitudeHold m_altHold;
    YawHold m_yawHold;
    Failsafe m_failsafe;
    AttitudeTrim m_trim;

    physics::BodyId m_droneBody;
    physics::BodyId m_groundBody;
    std::vector<physics::BodyId> m_obstacleBodies;
    std::vector<physics::BodyId> m_landingZoneBodies;
    float m_lastLandingXpTime = -1e9f;
    float m_lastBattery = 0;
    float m_gravity = 9.81f;  // magnitud, tomada de PhysicsSettings
    bool m_recovering = false;
    float m_hoverThrottle = 0.5f;  // fracción de empuje que compensa el peso
    // Empuje actual de cada motor: el motor y su ESC no responden a un
    // escalón instantáneo, así que sigue al objetivo con un retardo.
    float m_motorThrust[4] = {0, 0, 0, 0};
    // El par de reacción es lineal con el empuje, así que arrastra el mismo
    // retardo que los motores que lo producen.
    float m_yawTorque = 0.0f;
};

}  // namespace drone
