#include <catch2/catch_test_macros.hpp>

#include "core/Drone.h"
#include "core/GameConfig.h"

using drone::Drone;
using drone::GameConfig;
using drone::Vec3;

TEST_CASE("Drone initial state", "[Drone]") {
    GameConfig cfg;
    Drone d(cfg);
    REQUIRE(d.battery() == cfg.batteryMax);
    REQUIRE(d.position().length() == 0.0f);
    REQUIRE(d.velocity().length() == 0.0f);
    REQUIRE(d.isGrounded());
    REQUIRE(d.hasBattery());
}

TEST_CASE("Drone thrust input is clamped to [-1, 1] per axis", "[Drone]") {
    GameConfig cfg;
    Drone d(cfg);
    d.setThrustInput({5.0f, -3.0f, 0.5f});
    REQUIRE(d.thrustInput().x == 1.0f);
    REQUIRE(d.thrustInput().y == -1.0f);
    REQUIRE(d.thrustInput().z == 0.5f);
}

TEST_CASE("Drone battery never goes below 0 nor above max", "[Drone]") {
    GameConfig cfg;
    Drone d(cfg);
    d.drainBattery(1e6f);
    REQUIRE(d.battery() == 0.0f);
    REQUIRE_FALSE(d.hasBattery());
    d.drainBattery(-1e6f);
    REQUIRE(d.battery() == cfg.batteryMax);
}

TEST_CASE("Drone clampToGround stops downward motion", "[Drone]") {
    GameConfig cfg;
    Drone d(cfg);
    d.setPosition({0, -2.0f, 0});
    d.setVelocity({1.0f, -5.0f, 0});
    d.clampToGround();
    REQUIRE(d.position().y == 0.0f);
    REQUIRE(d.velocity().y == 0.0f);
    REQUIRE(d.velocity().x == 1.0f);
}

TEST_CASE("Drone reset restores initial state", "[Drone]") {
    GameConfig cfg;
    Drone d(cfg);
    d.setPosition({1, 2, 3});
    d.setVelocity({4, 5, 6});
    d.setThrustInput({1, 1, 1});
    d.drainBattery(50.0f);
    d.reset();
    REQUIRE(d.position().length() == 0.0f);
    REQUIRE(d.velocity().length() == 0.0f);
    REQUIRE(d.thrustInput().length() == 0.0f);
    REQUIRE(d.battery() == cfg.batteryMax);
}
