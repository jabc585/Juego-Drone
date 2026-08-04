#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "app/ConfigLoader.h"
#include "app/GameLogger.h"
#include "app/SaveManager.h"
#include "core/GameController.h"
#include "frontend/terminal/TerminalInput.h"
#include "frontend/terminal/TerminalRenderer.h"

#ifndef DRONE_VERSION
#define DRONE_VERSION "0.0.0-dev"
#endif

namespace {

void printHelp() {
    std::puts("DroneFlightSim " DRONE_VERSION
              "\n"
              "Uso: DroneFlightSim [--version] [--help]\n\n"
              "Controles:\n"
              "  W/A/S/D o flechas  mover\n"
              "  Q / E              ascender / descender\n"
              "  P                  pausa\n"
              "  F5                 guardar partida\n"
              "  F9                 cargar partida\n"
              "  X o Esc            salir\n"
              "  R                  reiniciar (tras fin de partida)");
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::puts("DroneFlightSim " DRONE_VERSION);
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0) {
            printHelp();
            return 0;
        }
    }

    drone::GameConfig config = drone::loadConfig("assets/config/game.toml");
    drone::validateConfig(config);

    drone::GameLogger logger;
    logger.info("Juego iniciado (v" DRONE_VERSION ")");

    drone::TerminalInput input;
    drone::TerminalRenderer renderer;
    drone::GameController game(input, renderer, config);

    logger.attach(game.getEventBus());

    // Carga automatica de partida guardada si existe
    if (auto old = drone::loadGame(drone::saveFilePath()); old.version > 0) {
        game.applyLoad(old);
        logger.info("Partida guardada cargada");
    }

    // Mismo bucle temporal que GameController::run(), pero intercalando el
    // manejo de guardado/carga (I/O que el core no puede hacer).
    using clock = std::chrono::steady_clock;
    auto last = clock::now();

    while (game.state() != drone::GameState::ShuttingDown) {
        const auto now = clock::now();
        const float frame = std::chrono::duration<float>(now - last).count();
        last = now;

        game.tick(frame);

        if (game.saveRequested()) {
            game.clearSaveRequest();
            auto data = drone::buildSaveData(game.getSnapshot());
            if (drone::saveGame(drone::saveFilePath(), data)) {
                game.getEventBus().publish({drone::EventType::GameSaved, 0});
            } else {
                logger.warn("Error al guardar partida");
            }
        }

        if (game.loadRequested()) {
            game.clearLoadRequest();
            auto data = drone::loadGame(drone::saveFilePath());
            if (data.version > 0) {
                game.applyLoad(data);
                game.getEventBus().publish({drone::EventType::GameLoaded, 0});
            } else {
                logger.warn("No se encontro partida guardada");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
