#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "core/EventBus.h"

using drone::Event;
using drone::EventBus;
using drone::EventType;

TEST_CASE("EventBus delivers to subscriber of the type", "[EventBus]") {
    EventBus bus;
    std::vector<float> received;
    bus.subscribe(EventType::LevelUp, [&](const Event& e) { received.push_back(e.value); });
    bus.publish({EventType::LevelUp, 2.0f});
    REQUIRE(received.size() == 1);
    REQUIRE(received[0] == 2.0f);
}

TEST_CASE("EventBus does not deliver other types", "[EventBus]") {
    EventBus bus;
    int calls = 0;
    bus.subscribe(EventType::Collision, [&](const Event&) { ++calls; });
    bus.publish({EventType::LevelUp, 2.0f});
    REQUIRE(calls == 0);
}

TEST_CASE("EventBus supports multiple subscribers in order", "[EventBus]") {
    EventBus bus;
    std::vector<int> order;
    bus.subscribe(EventType::BatteryLow, [&](const Event&) { order.push_back(1); });
    bus.subscribe(EventType::BatteryLow, [&](const Event&) { order.push_back(2); });
    bus.publish({EventType::BatteryLow, 15.0f});
    REQUIRE(order == std::vector<int>{1, 2});
}

TEST_CASE("EventBus publish without subscribers is safe", "[EventBus]") {
    EventBus bus;
    REQUIRE_NOTHROW(bus.publish({EventType::DroneUnlocked, 3.0f}));
}

TEST_CASE("EventBus subscribeAll receives every type", "[EventBus]") {
    EventBus bus;
    int calls = 0;
    bus.subscribeAll([&](const Event&) { ++calls; });
    bus.publish({EventType::LevelUp, 2.0f});
    bus.publish({EventType::Collision, 9.0f});
    REQUIRE(calls == 2);
}
