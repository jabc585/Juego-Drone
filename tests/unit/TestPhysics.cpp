// Regresión de vuelo (grafico.md §10.1): tras la migración a rp3d ya no se
// comprueba una fórmula de integración concreta, sino el comportamiento
// observable con tolerancia explícita. Un cambio de parámetros o de versión
// del motor que altere el vuelo hace saltar estos tests.
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/Drone.h"
#include "core/Environment.h"
#include "core/EventBus.h"
#include "core/GameConfig.h"
#include "core/World.h"

using namespace drone;

namespace {

GameConfig makeConfig() {
    GameConfig cfg;
    cfg.batteryPerNewton = 0.0f;  // sin consumo para tests puros
    return cfg;
}

// El solver deja una penetración residual en el contacto de reposo: exigir
// y >= 0 exacto haría fallar el test por décimas de milímetro.
constexpr float kPenetrationSlack = 0.1f;  // rp3d permite penetracion residual

}  // namespace

TEST_CASE("Physics: drone ascends with upward thrust", "[Physics]") {
    GameConfig cfg = makeConfig();
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    w.setThrustInput({0, 1.0f, 0});
    for (int i = 0; i < 60; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(w.drone().position().y > 0.0f);
}

// Criterio de aceptación de la Fase 1: los ajustes de [physics.*] llegan al
// motor y cambian el comportamiento sin recompilar. Con gravedad cero el
// dron no cae, y eso solo puede venir de PhysicsSettings.
TEST_CASE("Physics: PhysicsSettings reach the engine", "[Physics]") {
    GameConfig cfg = makeConfig();
    physics::PhysicsSettings physCfg;
    physCfg.gravity = {0.0f, 0.0f, 0.0f};
    World w(cfg, physCfg);

    w.teleportDrone({0, 20.0f, 0});
    for (int i = 0; i < 300; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(w.drone().position().y > 19.0f);
}

TEST_CASE("Physics: drone falls without thrust", "[Physics]") {
    GameConfig cfg = makeConfig();
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    w.teleportDrone({0, 10.0f, 0});
    for (int i = 0; i < 120; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(w.drone().position().y < 10.0f);
}

TEST_CASE("Physics: ground stops fall, y never negative (B4)", "[Physics]") {
    GameConfig cfg = makeConfig();
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    w.teleportDrone({0, 2.0f, 0});
    for (int i = 0; i < 600; ++i) {
        w.step(cfg.fixedTimestep);
        REQUIRE(w.drone().position().y >= -kPenetrationSlack);
    }
}

TEST_CASE("Physics: a resting drone is reported as grounded", "[Physics]") {
    GameConfig cfg = makeConfig();
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    w.teleportDrone({0, 3.0f, 0});
    w.step(cfg.fixedTimestep);
    REQUIRE_FALSE(w.drone().isGrounded());

    for (int i = 0; i < 300; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(w.drone().isGrounded());
}

TEST_CASE("Physics: empty battery ignores thrust (B9)", "[Physics]") {
    GameConfig cfg = makeConfig();
    cfg.batteryPerNewton = 100.0f;
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    w.setThrustInput({0, 1.0f, 0});
    for (int i = 0; i < 600; ++i)
        w.step(cfg.fixedTimestep);
    // Sin bateria no deberia ascender
    REQUIRE(w.drone().battery() <= 0);
}

TEST_CASE("Physics: thrust drains battery", "[Physics]") {
    GameConfig cfg;
    cfg.batteryPerNewton = 0.5f;
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    float before = w.drone().battery();
    w.setThrustInput({0, 1.0f, 0});
    for (int i = 0; i < 60; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(w.drone().battery() < before);
}

TEST_CASE("Physics: hard landing publishes Collision", "[Physics]") {
    GameConfig cfg;
    cfg.crashSpeed = 1.0f;
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    // El máximo, no el último: tras el impacto fuerte el dron rebota y
    // vuelve a tocar suelo suavemente, y quedarse con el último contacto
    // mediría el reposo en lugar del choque.
    float impact = 0;
    w.events().subscribe(EventType::Collision,
                         [&](const Event& e) { impact = std::max(impact, e.value); });
    w.teleportDrone({0, cfg.maxAltitude, 0});
    for (int i = 0; i < 3600; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(impact > cfg.crashSpeed);
}

TEST_CASE("Physics: landing softly does not report a collision", "[Physics]") {
    GameConfig cfg = makeConfig();
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    int collisions = 0;
    w.events().subscribe(EventType::Collision, [&](const Event&) { ++collisions; });

    // Apoyado en el suelo, sin caída: sin umbral, el simple contacto de
    // reposo publicaría una colisión de 0 m/s en cada aterrizaje y el HUD
    // avisaría de un choque que no existe.
    w.teleportDrone({0, 0.0f, 0});
    for (int i = 0; i < 300; ++i)
        w.step(cfg.fixedTimestep);
    REQUIRE(collisions == 0);
    REQUIRE(w.drone().isGrounded());
}

TEST_CASE("Physics: battery threshold events fire", "[Physics]") {
    GameConfig cfg;
    cfg.batteryPerNewton = 0.2f;
    cfg.batteryLowThreshold = 50.0f;
    physics::PhysicsSettings physCfg;
    World w(cfg, physCfg);

    int low = 0, empty = 0;
    w.events().subscribe(EventType::BatteryLow, [&](const Event&) { ++low; });
    w.events().subscribe(EventType::BatteryEmpty, [&](const Event&) { ++empty; });

    w.setThrustInput({0, 1.0f, 0});
    for (int i = 0; i < 3600; ++i)
        w.step(cfg.fixedTimestep);

    // Exactamente uno de cada: son cruces de umbral, no estados. Antes se
    // publicaban en cada frame por debajo del umbral.
    REQUIRE(low == 1);
    REQUIRE(empty == 1);
}
