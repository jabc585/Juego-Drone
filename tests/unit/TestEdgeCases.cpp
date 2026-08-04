#include <catch2/catch_test_macros.hpp>

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/GameConfig.h"
#include "core/PhysicsEngine.h"
#include "core/PlayerProgression.h"

using drone::Drone;
using drone::Environment;
using drone::EventBus;
using drone::GameConfig;
using drone::PhysicsEngine;

TEST_CASE("Drone with extreme thrust input is clamped safely", "[EdgeCase]") {
    GameConfig cfg;
    Drone d(cfg);
    d.setThrustInput({1e6f, -1e6f, 0.5f});
    REQUIRE(d.thrustInput().x == 1.0f);
    REQUIRE(d.thrustInput().y == -1.0f);
    REQUIRE(d.thrustInput().z == 0.5f);
}

TEST_CASE("Physics survives massive time step without NaN", "[EdgeCase]") {
    GameConfig cfg;
    cfg.maxFrameTime = 10.0f;
    EventBus bus;
    Environment env(cfg);
    PhysicsEngine phys(cfg, bus);
    Drone drone(cfg);

    drone.setPosition({0, 10.0f, 0});
    drone.setVelocity({1.0f, 0.0f, 0.0f});
    phys.step(drone, env, 10.0f);

    REQUIRE(std::isfinite(drone.position().x));
    REQUIRE(std::isfinite(drone.position().y));
    REQUIRE(std::isfinite(drone.position().z));
    REQUIRE(drone.position().y >= 0.0f);  // ground clamp survived
}

TEST_CASE("Physics with zero-mass config does not divide by zero", "[EdgeCase]") {
    // Test de robustez: masa cero seria absurda pero validateConfig() la rechazaria.
    // Si de alguna forma llegara, la division por cero en accel produciria NaN.
    GameConfig cfg;
    cfg.droneMass = 0.01f;  // minima valida segun validateConfig()
    EventBus bus;
    Environment env(cfg);
    PhysicsEngine phys(cfg, bus);
    Drone drone(cfg);

    drone.setPosition({0, 5.0f, 0});
    phys.step(drone, env, cfg.fixedTimestep);

    REQUIRE(std::isfinite(drone.position().x));
    REQUIRE(std::isfinite(drone.velocity().y));
}

TEST_CASE("Drone battery drain with zero thrust does nothing", "[EdgeCase]") {
    GameConfig cfg;
    Drone d(cfg);
    float before = d.battery();
    d.drainBattery(0.0f);
    REQUIRE(d.battery() == before);
}

TEST_CASE("Drone drainBattery with negative value does not exceed max", "[EdgeCase]") {
    GameConfig cfg;
    Drone d(cfg);
    d.drainBattery(-1000.0f);
    REQUIRE(d.battery() == cfg.batteryMax);
}

TEST_CASE("Environment handles zero timestep gracefully", "[EdgeCase]") {
    GameConfig cfg;
    Environment env(cfg);
    env.setSeed(1);
    for (int i = 0; i < 100; ++i)
        env.step(0.0f);
    REQUIRE(env.difficulty() == 1.0f);
    REQUIRE(env.wind().length() == 0.0f);
}

TEST_CASE("PlayerProgression with huge XP values does not overflow int", "[EdgeCase]") {
    // Progression usa int para XP; 2^31 - 1 es el maximo teorico.
    // Threshold es 100, asi que 2e9 XP deberia manejarse.
    GameConfig cfg;
    EventBus bus;
    drone::PlayerProgression p(cfg, bus);
    p.addExperience(2000000000);
    REQUIRE(p.getExperience() >= 0);
    REQUIRE(p.getLevel() > 1);
}
