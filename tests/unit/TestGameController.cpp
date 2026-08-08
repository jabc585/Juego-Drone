#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <queue>

#include "core/Commands.h"
#include "core/GameConfig.h"
#include "core/GameController.h"
#include "core/WorldState.h"
#include "frontend/IInputSource.h"
#include "frontend/IRenderer.h"
#include "physics/PhysicsSettings.h"

using drone::Command;
using drone::Event;
using drone::GameConfig;
using drone::GameController;
using drone::GameState;
using drone::IInputSource;
using drone::IRenderer;
using drone::WorldState;
using drone::physics::PhysicsSettings;

namespace {

struct TestInput : IInputSource {
    std::queue<Command> commands;
    Command poll() override {
        if (commands.empty())
            return Command::None;
        Command c = commands.front();
        commands.pop();
        return c;
    }
    void push(Command c) { commands.push(c); }
    void pushRepeat(Command c, int n) {
        for (int i = 0; i < n; ++i)
            commands.push(c);
    }
};

struct TestRenderer : IRenderer {
    WorldState lastState{};
    float lastAlpha = 0.0f;
    int drawCalls = 0;
    void draw(const WorldState& state, float alpha) override {
        lastState = state;
        lastAlpha = alpha;
        ++drawCalls;
    }
    void onEvent(const Event&) override {}
};

GameConfig makeConfig() {
    GameConfig cfg;
    cfg.fixedTimestep = 1.0f / 60.0f;
    cfg.thrustPulseSeconds = 0.35f;
    cfg.crashSpeed = 8.0f;
    cfg.batteryPerNewton = 0.0f;
    cfg.batteryMax = 100.0f;
    cfg.maxFrameTime = 0.25f;
    cfg.xpPerSecond = 0.0f;
    cfg.maxAltitude = 50.0f;
    cfg.worldHalfExtent = 100.0f;
    return cfg;
}

GameConfig crashConfig() {
    GameConfig c = makeConfig();
    // Con este umbral basta la caída de un salto de un metro para estrellarse.
    c.crashSpeed = 0.5f;
    return c;
}

// Asciende y deja caer hasta alcanzar GameOver (o agotar 30 s simulados).
// Nota: un tick está limitado por maxFrameTime, así que se simula frame a
// frame — tick(10.0f) NO simula 10 segundos.
void driveToGameOver(GameController& game, TestInput& input) {
    input.push(Command::Ascend);
    for (int i = 0; i < 1800 && game.state() == GameState::Playing; ++i)
        game.tick(1.0f / 60.0f);
}

}  // namespace

TEST_CASE("GameController starts in Playing state", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    REQUIRE(game.state() == GameState::Playing);
}

TEST_CASE("GameController::tick processes commands and advances time", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    cfg.xpPerSecond = 5.0f;
    GameController game(input, renderer, cfg, physCfg);
    input.pushRepeat(Command::Ascend, 1);
    game.tick(1.0f);
    REQUIRE(renderer.drawCalls > 0);
    REQUIRE(renderer.lastState.dronePosition.y > 0.0f);
}

TEST_CASE("GameController pauses and resumes with P key", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    REQUIRE(game.state() == GameState::Playing);
    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);
    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Playing);
}

TEST_CASE("GameController quit from Playing state", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    input.push(Command::Quit);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::ShuttingDown);
}

TEST_CASE("GameController: pause menu options work", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);
    input.push(Command::Option2);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Settings);
    input.push(Command::Ascend);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);
    input.push(Command::Option3);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::ShuttingDown);
}

TEST_CASE("GameController: hard crash triggers GameOver", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = crashConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    driveToGameOver(game, input);
    REQUIRE(game.state() == GameState::GameOver);
}

TEST_CASE("GameController: restart from GameOver", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = crashConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    driveToGameOver(game, input);
    REQUIRE(game.state() == GameState::GameOver);
    input.push(Command::Restart);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Playing);

    // El mundo vuelve al estado inicial. Sin reiniciar también el cuerpo de
    // rp3d, el dron reaparecía donde se estrelló: reset() tocaba el estado
    // de juego, pero el motor seguía teniendo la última posición.
    REQUIRE(std::fabs(renderer.lastState.dronePosition.y) < 0.05f);
    REQUIRE(renderer.lastState.battery == cfg.batteryMax);
}

TEST_CASE("GameController: handleCommand ignored in GameOver except restart/quit",
          "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = crashConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    driveToGameOver(game, input);
    REQUIRE(game.state() == GameState::GameOver);
    float yBefore = renderer.lastState.dronePosition.y;
    input.push(Command::Ascend);
    game.tick(1.0f);
    REQUIRE(game.state() == GameState::GameOver);
    REQUIRE(renderer.lastState.dronePosition.y <= yBefore + 0.1f);
}

TEST_CASE("GameController: accumulator respects max frame time clamp", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    cfg.maxFrameTime = 1.0f / 30.0f;
    GameController game(input, renderer, cfg, physCfg);
    game.tick(10.0f);
    REQUIRE(renderer.lastState.dronePosition.y > -10.0f);
}

TEST_CASE("GameController: shutdown from Paused via Quit", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);
    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);
    input.push(Command::Quit);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::ShuttingDown);
}

// --- Barra espaciadora: una pulsacion sube un metro, dos suben sin parar ---

namespace {

// Simula segundos de juego a paso fijo y devuelve la altura maxima vista.
float correr(GameController& game, TestRenderer& renderer, float dt, float segundos) {
    float pico = renderer.lastState.dronePosition.y;
    for (int i = 0; i < static_cast<int>(segundos / dt); ++i) {
        game.tick(dt);
        pico = std::fmax(pico, renderer.lastState.dronePosition.y);
    }
    return pico;
}

}  // namespace

TEST_CASE("Elevacion: una pulsacion sube un metro", "[GameController][elevacion]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);

    input.push(Command::Ascend);
    const float pico = correr(game, renderer, cfg.fixedTimestep, 2.5f);

    // Margen amplio: el corte se predice con la inercia y el retardo del motor.
    REQUIRE(pico > 0.8f);
    REQUIRE(pico < 1.3f);
}

TEST_CASE("Elevacion: sin tocar nada la gravedad lo baja", "[GameController][elevacion]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);

    input.push(Command::Ascend);
    const float pico = correr(game, renderer, cfg.fixedTimestep, 3.0f);
    REQUIRE(pico > 0.8f);
    REQUIRE(renderer.lastState.dronePosition.y < pico - 0.5f);
}

TEST_CASE("Elevacion: dos pulsaciones seguidas suben sin parar", "[GameController][elevacion]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);

    input.push(Command::Ascend);
    input.push(Command::Ascend);  // ambas en el mismo tick: doble pulsacion
    correr(game, renderer, cfg.fixedTimestep, 3.0f);

    // Muy por encima del metro de una pulsacion suelta.
    REQUIRE(renderer.lastState.dronePosition.y > 10.0f);
}

TEST_CASE("Elevacion: otra pulsacion corta el ascenso continuo", "[GameController][elevacion]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    PhysicsSettings physCfg;
    GameController game(input, renderer, cfg, physCfg);

    input.push(Command::Ascend);
    input.push(Command::Ascend);
    correr(game, renderer, cfg.fixedTimestep, 2.0f);

    // Pulsacion suelta (fuera de la ventana de doble): corta.
    input.push(Command::Ascend);
    const float pico = correr(game, renderer, cfg.fixedTimestep, 4.0f);
    REQUIRE(renderer.lastState.dronePosition.y < pico - 1.0f);
}
