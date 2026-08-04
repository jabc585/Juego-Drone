#include <catch2/catch_test_macros.hpp>
#include <queue>

#include "core/Commands.h"
#include "core/GameConfig.h"
#include "core/GameController.h"
#include "core/WorldState.h"
#include "frontend/IInputSource.h"
#include "frontend/IRenderer.h"

using drone::Command;
using drone::Event;
using drone::GameConfig;
using drone::GameController;
using drone::GameState;
using drone::IInputSource;
using drone::IRenderer;
using drone::WorldState;

namespace {

// Mock que entrega comandos de una cola FIFO.
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

// Mock que captura el ultimo estado renderizado.
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
    cfg.batteryPerNewton = 0.0f;  // sin consumo para simplificar tests
    cfg.batteryMax = 100.0f;
    cfg.maxFrameTime = 0.25f;
    cfg.xpPerSecond = 0.0f;
    return cfg;
}

}  // namespace

TEST_CASE("GameController starts in Playing state", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    GameController game(input, renderer, cfg);
    REQUIRE(game.state() == GameState::Playing);
}

TEST_CASE("GameController::tick processes commands and advances time", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    cfg.xpPerSecond = 5.0f;
    GameController game(input, renderer, cfg);

    // 1 segundo de thrust hacia arriba
    input.pushRepeat(Command::Ascend, 1);
    game.tick(1.0f);

    // Al menos 1 frame renderizado (60 steps en 1s)
    REQUIRE(renderer.drawCalls > 0);
    // El dron debio ascender
    REQUIRE(renderer.lastState.dronePosition.y > 0.0f);
}

TEST_CASE("GameController pauses and resumes with P key", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    GameController game(input, renderer, cfg);

    REQUIRE(game.state() == GameState::Playing);

    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);

    // P de nuevo reanuda
    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Playing);
}

TEST_CASE("GameController quit from Playing state", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    GameController game(input, renderer, cfg);

    input.push(Command::Quit);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::ShuttingDown);
}

TEST_CASE("GameController: pause menu options work", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    GameController game(input, renderer, cfg);

    // Entrar en pausa
    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);

    // Opcion 2: Settings
    input.push(Command::Option2);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Settings);

    // Cualquier tecla vuelve a Paused
    input.push(Command::Ascend);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);

    // Opcion 3: Salir
    input.push(Command::Option3);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::ShuttingDown);
}

namespace {

// Configuración que hace inevitable el choque: pulso de empuje largo para
// ganar altura y umbral de crash mínimo para que la caída posterior lo supere.
GameConfig crashConfig() {
    GameConfig cfg = makeConfig();
    cfg.crashSpeed = 0.5f;
    cfg.thrustPulseSeconds = 2.0f;
    return cfg;
}

// Asciende y deja caer hasta alcanzar GameOver (o agotar 30 s simulados).
// Nota: un tick está limitado por maxFrameTime, así que se simula frame a
// frame — tick(10.0f) NO simula 10 segundos.
void driveToGameOver(GameController& game, TestInput& input) {
    input.push(Command::Ascend);
    for (int i = 0; i < 1800 && game.state() == GameState::Playing; ++i) {
        game.tick(1.0f / 60.0f);
    }
}

}  // namespace

TEST_CASE("GameController: hard crash triggers GameOver", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = crashConfig();
    GameController game(input, renderer, cfg);

    driveToGameOver(game, input);

    REQUIRE(game.state() == GameState::GameOver);
}

TEST_CASE("GameController: restart from GameOver", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = crashConfig();
    GameController game(input, renderer, cfg);

    driveToGameOver(game, input);
    REQUIRE(game.state() == GameState::GameOver);

    input.push(Command::Restart);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Playing);
    // El mundo vuelve al estado inicial
    REQUIRE(renderer.lastState.dronePosition.y == 0.0f);
    REQUIRE(renderer.lastState.battery == cfg.batteryMax);
}

TEST_CASE("GameController: handleCommand ignored in GameOver except restart/quit",
          "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = crashConfig();
    GameController game(input, renderer, cfg);

    driveToGameOver(game, input);
    REQUIRE(game.state() == GameState::GameOver);

    // En GameOver el mundo está congelado: el empuje se ignora.
    const float yBefore = renderer.lastState.dronePosition.y;
    input.push(Command::Ascend);
    for (int i = 0; i < 60; ++i)
        game.tick(1.0f / 60.0f);
    REQUIRE(game.state() == GameState::GameOver);
    REQUIRE(renderer.lastState.dronePosition.y == yBefore);
}

TEST_CASE("GameController: accumulator respects max frame time clamp", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    cfg.maxFrameTime = 1.0f / 30.0f;  // clamp bajo
    GameController game(input, renderer, cfg);

    // Frame gigante (10s) — el clamp lo limita
    game.tick(10.0f);
    // El dron no deberia haber caido 10s * 9.81m/s = ~98m
    REQUIRE(renderer.lastState.dronePosition.y > -10.0f);
}

TEST_CASE("GameController: shutdown from Paused via Quit", "[GameController]") {
    TestInput input;
    TestRenderer renderer;
    GameConfig cfg = makeConfig();
    GameController game(input, renderer, cfg);

    input.push(Command::Pause);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::Paused);

    input.push(Command::Quit);
    game.tick(cfg.fixedTimestep);
    REQUIRE(game.state() == GameState::ShuttingDown);
}
