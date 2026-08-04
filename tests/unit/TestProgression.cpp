#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "core/EventBus.h"
#include "core/GameConfig.h"
#include "core/PlayerProgression.h"

using drone::Event;
using drone::EventBus;
using drone::EventType;
using drone::GameConfig;
using drone::PlayerProgression;

static GameConfig makeConfig() {
    GameConfig cfg;
    cfg.xpPerLevelBase = 100;
    cfg.unlockLevel = 3;
    return cfg;
}

TEST_CASE("Progression starts at level 1 with 0 XP", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    REQUIRE(p.getLevel() == 1);
    REQUIRE(p.getExperience() == 0);
    REQUIRE(p.getExperienceToNext() == 100);
}

TEST_CASE("100 XP reaches level 2", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    p.addExperience(100);
    REQUIRE(p.getLevel() == 2);
}

TEST_CASE("250 XP reaches level 3 with residual (regression B7)", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    p.addExperience(250);
    REQUIRE(p.getLevel() == 3);
    REQUIRE(p.getExperience() == 50);
}

TEST_CASE("Non-positive XP is ignored", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    p.addExperience(0);
    p.addExperience(-50);
    REQUIRE(p.getLevel() == 1);
    REQUIRE(p.getExperience() == 0);
}

TEST_CASE("Drone unlocked at level 3", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    p.addExperience(100);
    p.addExperience(150);
    REQUIRE(p.getLevel() >= 3);
    REQUIRE(p.unlockDrone().find("desbloqueado") != std::string::npos);
}

TEST_CASE("No drone unlocked below level 3", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    REQUIRE(p.unlockDrone().find("Sube de nivel") != std::string::npos);
}

TEST_CASE("Progression publishes LevelUp and DroneUnlocked events (R7)", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    std::vector<EventType> received;
    bus.subscribeAll([&](const Event& e) { received.push_back(e.type); });

    PlayerProgression p(cfg, bus);
    p.addExperience(250);

    REQUIRE(received == std::vector<EventType>{EventType::LevelUp, EventType::LevelUp,
                                               EventType::DroneUnlocked});
}

TEST_CASE("Progression reset returns to level 1", "[Progression]") {
    GameConfig cfg = makeConfig();
    EventBus bus;
    PlayerProgression p(cfg, bus);
    p.addExperience(500);
    p.reset();
    REQUIRE(p.getLevel() == 1);
    REQUIRE(p.getExperience() == 0);
}
