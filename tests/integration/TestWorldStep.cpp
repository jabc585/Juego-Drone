#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/GameConfig.h"
#include "core/World.h"

using drone::Event;
using drone::EventType;
using drone::GameConfig;
using drone::Obstacle;
using drone::Vec3;
using drone::World;
using drone::WorldState;

namespace {

void run(World& w, int steps, const GameConfig& cfg) {
    for (int i = 0; i < steps; ++i)
        w.step(cfg.fixedTimestep);
}

}  // namespace

TEST_CASE("World: invariants hold over a full accelerated flight", "[World][integration]") {
    GameConfig cfg;
    World w(cfg);
    w.environment().loadEnvironment("Ciudad Futurista");
    w.environment().setSeed(1234);

    for (int second = 0; second < 60; ++second) {
        const float dir = (second % 2 == 0) ? 1.0f : -0.3f;
        w.setThrustInput({0.4f * dir, 0.8f, 0.4f});
        for (int i = 0; i < 60; ++i) {
            w.step(cfg.fixedTimestep);
            const Vec3 p = w.drone().position();
            REQUIRE(p.y >= 0.0f);
            REQUIRE(p.y <= cfg.maxAltitude);
            REQUIRE(std::fabs(p.x) <= cfg.worldHalfExtent);
            REQUIRE(std::fabs(p.z) <= cfg.worldHalfExtent);
            REQUIRE(w.drone().battery() >= 0.0f);
            REQUIRE(w.drone().battery() <= cfg.batteryMax);
        }
    }
}

TEST_CASE("World: same seed and commands produce identical trajectory", "[World][integration]") {
    GameConfig cfg;
    auto simulate = [&]() {
        World w(cfg);
        w.environment().loadEnvironment("Ciudad Futurista");
        w.environment().setSeed(42);
        w.setThrustInput({0.3f, 0.7f, 0.5f});
        for (int i = 0; i < 1800; ++i)
            w.step(cfg.fixedTimestep);
        return w.drone().position();
    };
    const Vec3 a = simulate();
    const Vec3 b = simulate();
    REQUIRE(a.x == b.x);
    REQUIRE(a.y == b.y);
    REQUIRE(a.z == b.z);
}

TEST_CASE("World: wind gusts appear over time (B6 closed)", "[World][integration]") {
    GameConfig cfg;
    World w(cfg);
    w.environment().setSeed(7);
    float maxWind = 0.0f;
    for (int i = 0; i < 60 * 30; ++i) {
        w.step(cfg.fixedTimestep);
        maxWind = std::max(maxWind, w.environment().wind().length());
    }
    REQUIRE(maxWind > 0.5f);
}

TEST_CASE("World: difficulty ramps with time (D3 closed)", "[World][integration]") {
    GameConfig cfg;
    World w(cfg);
    REQUIRE(w.environment().difficulty() == 1.0f);
    run(w, 60 * 60, cfg);
    REQUIRE(w.environment().difficulty() > 1.5f);
}

TEST_CASE("World: obstacle collision pushes the drone out and notifies", "[World][integration]") {
    GameConfig cfg;
    World w(cfg);
    w.environment().loadEnvironment("Ciudad Futurista");
    int collisions = 0;
    w.events().subscribe(EventType::Collision, [&](const Event&) { ++collisions; });

    w.drone().setPosition({8.0f, 5.0f, 4.0f});
    w.drone().setVelocity({0, 0, 6.0f});
    w.setThrustInput({0, 0.6f, 0.8f});
    run(w, 240, cfg);

    REQUIRE(collisions > 0);
    const Vec3 p = w.drone().position();
    const bool insideX = std::fabs(p.x - 8.0f) < 1.0f + cfg.droneRadius;
    const bool insideY = std::fabs(p.y - 5.0f) < 5.0f + cfg.droneRadius;
    const bool insideZ = std::fabs(p.z - 8.0f) < 1.0f + cfg.droneRadius;
    REQUIRE_FALSE((insideX && insideY && insideZ));
}

TEST_CASE("World: snapshot exposes the same obstacles the physics collides with",
          "[World][integration]") {
    GameConfig cfg;
    World w(cfg);
    // Sin nivel cargado no hay geometría que dibujar.
    REQUIRE(w.snapshot().obstacles.empty());

    w.environment().loadEnvironment("Ciudad Futurista");
    const WorldState s = w.snapshot();

    // El frontend debe dibujar exactamente lo que el motor colisiona: si el
    // renderer usa su propia copia, la escena miente sobre el mundo real.
    REQUIRE(s.obstacles.size() == w.environment().obstacles().size());
    REQUIRE_FALSE(s.obstacles.empty());
    for (std::size_t i = 0; i < s.obstacles.size(); ++i) {
        const Obstacle& expected = w.environment().obstacles()[i];
        REQUIRE(s.obstacles[i].center.x == expected.center.x);
        REQUIRE(s.obstacles[i].center.y == expected.center.y);
        REQUIRE(s.obstacles[i].center.z == expected.center.z);
        REQUIRE(s.obstacles[i].size.x == expected.size.x);
        REQUIRE(s.obstacles[i].size.y == expected.size.y);
        REQUIRE(s.obstacles[i].size.z == expected.size.z);
    }
}

TEST_CASE("World: reset restores a fresh deterministic world", "[World][integration]") {
    GameConfig cfg;
    World w(cfg);
    w.environment().loadEnvironment("Ciudad Futurista");
    w.environment().setSeed(99);
    w.setThrustInput({0.5f, 1.0f, 0.2f});
    run(w, 600, cfg);
    REQUIRE(w.simTime() > 9.9f);

    w.reset();
    REQUIRE(w.simTime() == 0.0f);
    REQUIRE(w.drone().position().length() == 0.0f);
    REQUIRE(w.drone().battery() == cfg.batteryMax);
    REQUIRE(w.environment().difficulty() == 1.0f);
}
