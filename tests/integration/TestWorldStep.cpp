#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/Config.h"
#include "core/World.h"

using namespace drone;
namespace cfg = drone::config;

namespace {

void run(World& w, int steps) {
    for (int i = 0; i < steps; ++i)
        w.step(cfg::kFixedTimestep);
}

}  // namespace

TEST_CASE("World: invariants hold over a full accelerated flight", "[World][integration]") {
    World w;
    w.environment().loadEnvironment("Ciudad Futurista");
    w.environment().setSeed(1234);

    // 60 s de vuelo alternando empuje cada segundo.
    for (int second = 0; second < 60; ++second) {
        const float dir = (second % 2 == 0) ? 1.0f : -0.3f;
        w.setThrustInput({0.4f * dir, 0.8f, 0.4f});
        for (int i = 0; i < 60; ++i) {
            w.step(cfg::kFixedTimestep);
            const Vec3 p = w.drone().position();
            REQUIRE(p.y >= 0.0f);
            REQUIRE(p.y <= cfg::kMaxAltitude);
            REQUIRE(std::fabs(p.x) <= cfg::kWorldHalfExtent);
            REQUIRE(std::fabs(p.z) <= cfg::kWorldHalfExtent);
            REQUIRE(w.drone().battery() >= 0.0f);
            REQUIRE(w.drone().battery() <= cfg::kBatteryMax);
        }
    }
}

TEST_CASE("World: same seed and commands produce identical trajectory", "[World][integration]") {
    auto simulate = []() {
        World w;
        w.environment().loadEnvironment("Ciudad Futurista");
        w.environment().setSeed(42);
        w.setThrustInput({0.3f, 0.7f, 0.5f});
        for (int i = 0; i < 1800; ++i)
            w.step(cfg::kFixedTimestep);
        return w.drone().position();
    };
    const Vec3 a = simulate();
    const Vec3 b = simulate();
    REQUIRE(a.x == b.x);
    REQUIRE(a.y == b.y);
    REQUIRE(a.z == b.z);
}

TEST_CASE("World: wind gusts appear over time (B6 closed)", "[World][integration]") {
    World w;
    w.environment().setSeed(7);
    float maxWind = 0.0f;
    for (int i = 0; i < 60 * 30; ++i) {
        w.step(cfg::kFixedTimestep);
        maxWind = std::max(maxWind, w.environment().wind().length());
    }
    REQUIRE(maxWind > 0.5f);
}

TEST_CASE("World: difficulty ramps with time (D3 closed)", "[World][integration]") {
    World w;
    REQUIRE(w.environment().difficulty() == 1.0f);
    run(w, 60 * 60);  // 60 s
    REQUIRE(w.environment().difficulty() > 1.5f);
}

TEST_CASE("World: obstacle collision pushes the drone out and notifies", "[World][integration]") {
    World w;
    w.environment().loadEnvironment("Ciudad Futurista");
    int collisions = 0;
    w.events().subscribe(EventType::Collision, [&](const Event&) { ++collisions; });

    // Vuela directo contra el primer obstáculo ({8,5,8} tamaño {2,10,2}).
    w.drone().setPosition({8.0f, 5.0f, 4.0f});
    w.drone().setVelocity({0, 0, 6.0f});
    w.setThrustInput({0, 0.6f, 0.8f});
    run(w, 240);

    REQUIRE(collisions > 0);
    // El dron nunca queda dentro del AABB.
    const Vec3 p = w.drone().position();
    const bool insideX = std::fabs(p.x - 8.0f) < 1.0f + cfg::kDroneRadius;
    const bool insideY = std::fabs(p.y - 5.0f) < 5.0f + cfg::kDroneRadius;
    const bool insideZ = std::fabs(p.z - 8.0f) < 1.0f + cfg::kDroneRadius;
    REQUIRE_FALSE((insideX && insideY && insideZ));
}

TEST_CASE("World: reset restores a fresh deterministic world", "[World][integration]") {
    World w;
    w.environment().loadEnvironment("Ciudad Futurista");
    w.environment().setSeed(99);
    w.setThrustInput({0.5f, 1.0f, 0.2f});
    run(w, 600);
    REQUIRE(w.simTime() > 9.9f);

    w.reset();
    REQUIRE(w.simTime() == 0.0f);
    REQUIRE(w.drone().position().length() == 0.0f);
    REQUIRE(w.drone().battery() == cfg::kBatteryMax);
    REQUIRE(w.environment().difficulty() == 1.0f);
}
