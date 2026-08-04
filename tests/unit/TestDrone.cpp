#include <catch2/catch_test_macros.hpp>

#include "core/Config.h"
#include "core/Drone.h"

using drone::Drone;
using drone::Vec3;

TEST_CASE("Drone initial state", "[Drone]") {
    Drone d;
    REQUIRE(d.battery() == drone::config::kBatteryMax);
    REQUIRE(d.position().length() == 0.0f);
    REQUIRE(d.velocity().length() == 0.0f);
    REQUIRE(d.isGrounded());
    REQUIRE(d.hasBattery());
}

TEST_CASE("Drone thrust input is clamped to [-1, 1] per axis", "[Drone]") {
    Drone d;
    d.setThrustInput({5.0f, -3.0f, 0.5f});
    REQUIRE(d.thrustInput().x == 1.0f);
    REQUIRE(d.thrustInput().y == -1.0f);
    REQUIRE(d.thrustInput().z == 0.5f);
}

TEST_CASE("Drone battery never goes below 0 nor above max", "[Drone]") {
    Drone d;
    d.drainBattery(1e6f);
    REQUIRE(d.battery() == 0.0f);
    REQUIRE_FALSE(d.hasBattery());
    d.drainBattery(-1e6f);  // "recarga" también respeta el tope
    REQUIRE(d.battery() == drone::config::kBatteryMax);
}

TEST_CASE("Drone clampToGround stops downward motion", "[Drone]") {
    Drone d;
    d.setPosition({0, -2.0f, 0});
    d.setVelocity({1.0f, -5.0f, 0});
    d.clampToGround();
    REQUIRE(d.position().y == 0.0f);
    REQUIRE(d.velocity().y == 0.0f);
    REQUIRE(d.velocity().x == 1.0f);  // la componente horizontal se conserva
}

TEST_CASE("Drone reset restores initial state", "[Drone]") {
    Drone d;
    d.setPosition({1, 2, 3});
    d.setVelocity({4, 5, 6});
    d.setThrustInput({1, 1, 1});
    d.drainBattery(50.0f);
    d.reset();
    REQUIRE(d.position().length() == 0.0f);
    REQUIRE(d.velocity().length() == 0.0f);
    REQUIRE(d.thrustInput().length() == 0.0f);
    REQUIRE(d.battery() == drone::config::kBatteryMax);
}
