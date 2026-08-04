#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/Config.h"
#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/PhysicsEngine.h"

using namespace drone;
namespace cfg = drone::config;

namespace {

// Entorno recién construido: sin obstáculos cargados y viento cero mientras
// no se llame a step(), lo que aísla la física del dron.
struct Rig {
    EventBus bus;
    Environment env;
    PhysicsEngine physics{bus};
    Drone drone;

    void steps(int n) {
        for (int i = 0; i < n; ++i)
            physics.step(drone, env, cfg::kFixedTimestep);
    }
};

}  // namespace

TEST_CASE("Physics: without thrust the drone falls", "[Physics]") {
    Rig r;
    r.drone.setPosition({0, 10.0f, 0});
    r.steps(60);  // 1 s
    REQUIRE(r.drone.position().y < 10.0f);
    REQUIRE(r.drone.velocity().y < 0.0f);
}

TEST_CASE("Physics: hover thrust equal to weight holds altitude", "[Physics]") {
    Rig r;
    const float hover = cfg::kGravity * cfg::kDroneMass / cfg::kMaxThrust;
    r.drone.setPosition({0, 5.0f, 0});
    r.drone.setThrustInput({0, hover, 0});
    r.steps(600);  // 10 s
    REQUIRE(std::fabs(r.drone.position().y - 5.0f) < 0.2f);
}

TEST_CASE("Physics: drag limits fall speed to terminal velocity", "[Physics]") {
    Rig r;
    r.drone.setPosition({0, cfg::kMaxAltitude, 0});
    r.steps(600);
    const float terminal = cfg::kGravity * cfg::kDroneMass / cfg::kDragCoefficient;
    REQUIRE(std::fabs(r.drone.velocity().y) <= terminal + 0.5f);
}

TEST_CASE("Physics: ground stops the fall, y never negative (B4)", "[Physics]") {
    Rig r;
    r.drone.setPosition({0, 0.5f, 0});
    for (int i = 0; i < 300; ++i) {
        r.physics.step(r.drone, r.env, cfg::kFixedTimestep);
        REQUIRE(r.drone.position().y >= 0.0f);
    }
    REQUIRE(r.drone.position().y == 0.0f);
    REQUIRE(r.drone.velocity().y == 0.0f);
}

TEST_CASE("Physics: empty battery ignores thrust (B9)", "[Physics]") {
    Rig r;
    r.drone.drainBattery(cfg::kBatteryMax);
    r.drone.setThrustInput({0, 1.0f, 0});
    r.steps(120);
    REQUIRE(r.drone.position().y == 0.0f);  // nunca despega
}

TEST_CASE("Physics: thrust drains battery proportionally", "[Physics]") {
    Rig r;
    r.drone.setThrustInput({0, 1.0f, 0});
    r.steps(60);  // 1 s a empuje máximo
    const float expectedDrain = cfg::kMaxThrust * cfg::kBatteryPerNewton;
    REQUIRE(std::fabs((cfg::kBatteryMax - r.drone.battery()) - expectedDrain) < 0.01f);
}

TEST_CASE("Physics: hard landing publishes Collision event", "[Physics]") {
    Rig r;
    float impact = 0.0f;
    r.bus.subscribe(EventType::Collision, [&](const Event& e) { impact = e.value; });
    r.drone.setPosition({0, cfg::kMaxAltitude, 0});
    r.steps(1200);  // 20 s: caída libre y choque
    REQUIRE(impact > cfg::kCrashSpeed);
}

TEST_CASE("Physics: battery threshold events fire once", "[Physics]") {
    Rig r;
    int low = 0;
    int empty = 0;
    r.bus.subscribe(EventType::BatteryLow, [&](const Event&) { ++low; });
    r.bus.subscribe(EventType::BatteryEmpty, [&](const Event&) { ++empty; });
    r.drone.drainBattery(cfg::kBatteryMax - cfg::kBatteryLowThreshold - 0.1f);
    r.drone.setThrustInput({0, 1.0f, 0});
    r.steps(60 * 60);  // hasta agotarla
    REQUIRE(low == 1);
    REQUIRE(empty == 1);
    REQUIRE(r.drone.battery() == 0.0f);
}

TEST_CASE("Physics: world bounds clamp position", "[Physics]") {
    Rig r;
    r.drone.setPosition({cfg::kWorldHalfExtent - 0.1f, 5.0f, 0});
    r.drone.setVelocity({50.0f, 0, 0});
    r.steps(120);
    REQUIRE(r.drone.position().x <= cfg::kWorldHalfExtent);
}
