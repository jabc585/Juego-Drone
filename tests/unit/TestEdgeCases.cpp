#include <catch2/catch_test_macros.hpp>

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/GameConfig.h"
#include "core/PlayerProgression.h"

using drone::Drone;
using drone::Environment;
using drone::EventBus;
using drone::GameConfig;

TEST_CASE("Drone with extreme thrust input is clamped safely", "[EdgeCase]") {
    GameConfig cfg;
    Drone d(cfg);
    d.setThrustInput({1e6f, -1e6f, 0.5f});
    REQUIRE(d.thrustInput().x == 1.0f);
    REQUIRE(d.thrustInput().y == -1.0f);
    REQUIRE(d.thrustInput().z == 0.5f);
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
    GameConfig cfg;
    EventBus bus;
    drone::PlayerProgression p(cfg, bus);
    p.addExperience(2000000000);
    REQUIRE(p.getExperience() >= 0);
    REQUIRE(p.getLevel() > 1);
}
