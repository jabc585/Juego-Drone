#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/GameConfig.h"
#include "core/PhysicsEngine.h"

using drone::Drone;
using drone::Environment;
using drone::Event;
using drone::EventBus;
using drone::EventType;
using drone::GameConfig;
using drone::PhysicsEngine;

namespace {

struct Rig {
    GameConfig cfg;
    EventBus bus;
    Environment env{cfg};
    PhysicsEngine physics{cfg, bus};
    Drone drone{cfg};

    void steps(int n) {
        for (int i = 0; i < n; ++i)
            physics.step(drone, env, cfg.fixedTimestep);
    }
};

}  // namespace

TEST_CASE("Physics: without thrust the drone falls", "[Physics]") {
    Rig r;
    r.drone.setPosition({0, 10.0f, 0});
    r.steps(60);
    REQUIRE(r.drone.position().y < 10.0f);
    REQUIRE(r.drone.velocity().y < 0.0f);
}

TEST_CASE("Physics: hover thrust equal to weight holds altitude", "[Physics]") {
    Rig r;
    const float hover = r.cfg.gravity * r.cfg.droneMass / r.cfg.maxThrust;
    r.drone.setPosition({0, 5.0f, 0});
    r.drone.setThrustInput({0, hover, 0});
    r.steps(600);
    REQUIRE(std::fabs(r.drone.position().y - 5.0f) < 0.2f);
}

TEST_CASE("Physics: drag limits fall speed to terminal velocity", "[Physics]") {
    Rig r;
    r.drone.setPosition({0, r.cfg.maxAltitude, 0});
    r.steps(600);
    const float terminal = r.cfg.gravity * r.cfg.droneMass / r.cfg.dragCoefficient;
    REQUIRE(std::fabs(r.drone.velocity().y) <= terminal + 0.5f);
}

TEST_CASE("Physics: ground stops the fall, y never negative (B4)", "[Physics]") {
    Rig r;
    r.drone.setPosition({0, 0.5f, 0});
    for (int i = 0; i < 300; ++i) {
        r.physics.step(r.drone, r.env, r.cfg.fixedTimestep);
        REQUIRE(r.drone.position().y >= 0.0f);
    }
    REQUIRE(r.drone.position().y == 0.0f);
    REQUIRE(r.drone.velocity().y == 0.0f);
}

TEST_CASE("Physics: empty battery ignores thrust (B9)", "[Physics]") {
    Rig r;
    r.drone.drainBattery(r.cfg.batteryMax);
    r.drone.setThrustInput({0, 1.0f, 0});
    r.steps(120);
    REQUIRE(r.drone.position().y == 0.0f);
}

TEST_CASE("Physics: thrust drains battery proportionally", "[Physics]") {
    Rig r;
    r.drone.setThrustInput({0, 1.0f, 0});
    r.steps(60);
    const float expectedDrain = r.cfg.maxThrust * r.cfg.batteryPerNewton;
    REQUIRE(std::fabs((r.cfg.batteryMax - r.drone.battery()) - expectedDrain) < 0.01f);
}

TEST_CASE("Physics: hard landing publishes Collision event", "[Physics]") {
    Rig r;
    float impact = 0.0f;
    r.bus.subscribe(EventType::Collision, [&](const Event& e) { impact = e.value; });
    r.drone.setPosition({0, r.cfg.maxAltitude, 0});
    r.steps(1200);
    REQUIRE(impact > r.cfg.crashSpeed);
}

TEST_CASE("Physics: battery threshold events fire once", "[Physics]") {
    Rig r;
    int low = 0, empty = 0;
    r.bus.subscribe(EventType::BatteryLow, [&](const Event&) { ++low; });
    r.bus.subscribe(EventType::BatteryEmpty, [&](const Event&) { ++empty; });
    r.drone.drainBattery(r.cfg.batteryMax - r.cfg.batteryLowThreshold - 0.1f);
    r.drone.setThrustInput({0, 1.0f, 0});
    r.steps(60 * 60);
    REQUIRE(low == 1);
    REQUIRE(empty == 1);
    REQUIRE(r.drone.battery() == 0.0f);
}

TEST_CASE("Physics: world bounds clamp position", "[Physics]") {
    Rig r;
    r.drone.setPosition({r.cfg.worldHalfExtent - 0.1f, 5.0f, 0});
    r.drone.setVelocity({50.0f, 0, 0});
    r.steps(120);
    REQUIRE(r.drone.position().x <= r.cfg.worldHalfExtent);
}
