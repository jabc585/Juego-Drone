#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "core/Config.h"
#include "core/EventBus.h"
#include "core/PlayerProgression.h"

using drone::Event;
using drone::EventBus;
using drone::EventType;
using drone::PlayerProgression;

TEST_CASE("Progression starts at level 1 with 0 XP", "[Progression]") {
    PlayerProgression p;
    REQUIRE(p.getLevel() == 1);
    REQUIRE(p.getExperience() == 0);
    REQUIRE(p.getExperienceToNext() == drone::config::kXPPerLevelBase);
}

TEST_CASE("100 XP reaches level 2", "[Progression]") {
    PlayerProgression p;
    p.addExperience(100);
    REQUIRE(p.getLevel() == 2);
}

TEST_CASE("250 XP reaches level 3 with residual (regression B7)", "[Progression]") {
    PlayerProgression p;
    p.addExperience(250);
    REQUIRE(p.getLevel() == 3);
    REQUIRE(p.getExperience() == 50);
}

TEST_CASE("Non-positive XP is ignored", "[Progression]") {
    PlayerProgression p;
    p.addExperience(0);
    p.addExperience(-50);
    REQUIRE(p.getLevel() == 1);
    REQUIRE(p.getExperience() == 0);
}

TEST_CASE("Drone unlocked at level 3", "[Progression]") {
    PlayerProgression p;
    p.addExperience(100);
    p.addExperience(150);
    REQUIRE(p.getLevel() >= 3);
    REQUIRE(p.unlockDrone().find("desbloqueado") != std::string::npos);
}

TEST_CASE("No drone unlocked below level 3", "[Progression]") {
    PlayerProgression p;
    REQUIRE(p.unlockDrone().find("Sube de nivel") != std::string::npos);
}

TEST_CASE("Progression publishes LevelUp and DroneUnlocked events (R7)", "[Progression]") {
    EventBus bus;
    std::vector<EventType> received;
    bus.subscribeAll([&](const Event& e) { received.push_back(e.type); });

    PlayerProgression p(&bus);
    p.addExperience(250);  // nivel 1 → 3: dos LevelUp + un DroneUnlocked

    REQUIRE(received == std::vector<EventType>{EventType::LevelUp, EventType::LevelUp,
                                               EventType::DroneUnlocked});
}

TEST_CASE("Progression reset returns to level 1", "[Progression]") {
    PlayerProgression p;
    p.addExperience(500);
    p.reset();
    REQUIRE(p.getLevel() == 1);
    REQUIRE(p.getExperience() == 0);
}
