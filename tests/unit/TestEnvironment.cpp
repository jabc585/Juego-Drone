#include <catch2/catch_test_macros.hpp>

#include "core/Environment.h"
#include "core/GameConfig.h"
#include "core/math/Vec3.h"

using drone::Environment;
using drone::GameConfig;
using drone::Vec3;

TEST_CASE("Environment starts with wind zero and difficulty 1", "[Environment]") {
    GameConfig cfg;
    Environment env(cfg);
    REQUIRE(env.wind().length() == 0.0f);
    REQUIRE(env.difficulty() == 1.0f);
}

TEST_CASE("Environment same seed produces same gusts", "[Environment]") {
    GameConfig cfg;
    cfg.gustMinInterval = 0.1f;
    cfg.gustMaxInterval = 0.1f;

    Environment a(cfg);
    Environment b(cfg);
    a.setSeed(42);
    b.setSeed(42);

    for (int i = 0; i < 600; ++i) {
        a.step(cfg.fixedTimestep);
        b.step(cfg.fixedTimestep);
    }

    REQUIRE(a.wind().x == b.wind().x);
    REQUIRE(a.wind().y == b.wind().y);
    REQUIRE(a.wind().z == b.wind().z);
}

TEST_CASE("Environment different seeds produce different gusts", "[Environment]") {
    GameConfig cfg;
    cfg.gustMinInterval = 0.1f;
    cfg.gustMaxInterval = 0.1f;

    Environment a(cfg);
    Environment b(cfg);
    a.setSeed(42);
    b.setSeed(99);

    for (int i = 0; i < 600; ++i) {
        a.step(cfg.fixedTimestep);
        b.step(cfg.fixedTimestep);
    }

    bool different =
        (a.wind().x != b.wind().x || a.wind().y != b.wind().y || a.wind().z != b.wind().z);
    REQUIRE(different);
}

TEST_CASE("Environment difficulty ramps with time", "[Environment]") {
    GameConfig cfg;
    cfg.difficultyRamp = 0.1f;
    Environment env(cfg);
    REQUIRE(env.difficulty() == 1.0f);
    for (int i = 0; i < 600; ++i)
        env.step(cfg.fixedTimestep);
    REQUIRE(env.difficulty() > 1.5f);
}

TEST_CASE("Environment wind smoothing prevents jumps", "[Environment]") {
    GameConfig cfg;
    cfg.windSmoothing = 2.0f;
    Environment env(cfg);
    env.setSeed(7);

    Vec3 prevWind;
    for (int i = 0; i < 600; ++i) {
        env.step(cfg.fixedTimestep);
        Vec3 w = env.wind();
        // El cambio maximo en 1/60s esta limitado por el suavizado
        float change = (w - prevWind).length();
        REQUIRE(change < 10.0f);
        prevWind = w;
    }
}

TEST_CASE("Environment reset returns to initial state", "[Environment]") {
    GameConfig cfg;
    Environment env(cfg);
    env.setSeed(42);
    for (int i = 0; i < 1200; ++i)
        env.step(cfg.fixedTimestep);
    REQUIRE(env.difficulty() > 1.0f);

    env.reset();
    REQUIRE(env.difficulty() == 1.0f);
    // Despues de reset con misma semilla, debe ser determinista
    Environment fresh(cfg);
    fresh.setSeed(42);
    for (int i = 0; i < 60; ++i) {
        env.step(cfg.fixedTimestep);
        fresh.step(cfg.fixedTimestep);
    }
    REQUIRE(env.wind().x == fresh.wind().x);
}
