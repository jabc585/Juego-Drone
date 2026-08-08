#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>

#include "app/ConfigLoader.h"
#include "app/GameLogger.h"
#include "app/SaveManager.h"
#include "core/GameController.h"
#include "frontend/IInputSource.h"
#include "frontend/IRenderer.h"
#include "frontend/terminal/TerminalInput.h"
#include "frontend/terminal/TerminalRenderer.h"

#ifdef DRONE_HAS_RAYLIB
#include "frontend/raylib/RaylibInput.h"
#include "frontend/raylib/RaylibRenderer.h"
#include "frontend/raylib/RaylibViewState.h"
#endif

#ifndef DRONE_VERSION
#define DRONE_VERSION "0.0.0-dev"
#endif

namespace {

void printHelp() {
    std::puts("DroneFlightSim " DRONE_VERSION
              "\n"
              "Uso: DroneFlightSim [--version] [--help]"
#ifdef DRONE_HAS_RAYLIB
              " [--gui]"
#endif
              "\n  --gui    usar frontend grafico (raylib) en vez de terminal\n\n"
              "Controles de vuelo:\n"
              "  W/A/S/D o flechas  mover\n"
              "  Espacio            subir un metro; dos veces seguidas, subir\n"
              "                     sin parar; otra pulsacion lo corta. Para\n"
              "                     bajar basta con soltar: manda la gravedad\n"
              "  H                  mantener altitud\n"
              "  F1 / F2            trim de cabeceo\n"
              "  F3 / F4            trim de alabeo\n"
              "  P                  pausa\n"
              "  F5                 guardar partida\n"
              "  F9                 cargar partida\n"
              "  X o Esc            salir\n"
              "  R                  reiniciar (tras fin de partida)\n\n"
              "Camara (modo --gui):\n"
              "  F12                alternar Follow / Free / Orbit\n"
              "  F11                pantalla completa\n"
              "  ClickDer + arrastre orbitar (modo Orbit)\n"
              "  Rueda raton        zoom (modo Orbit)\n"
              "  WASD               mover (modo Free)");
}

}  // namespace

int main(int argc, char** argv) {
    bool useGui = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::puts("DroneFlightSim " DRONE_VERSION);
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0) {
            printHelp();
            return 0;
        }
        if (std::strcmp(argv[i], "--gui") == 0) {
#ifdef DRONE_HAS_RAYLIB
            useGui = true;
#else
            std::fputs("Frontend grafico no compilado (raylib no encontrada).\n", stderr);
            return 1;
#endif
        }
    }

    drone::GameConfig config = drone::loadConfig("assets/config/game.toml");
    drone::validateConfig(config);
    auto physCfg = drone::loadPhysicsConfig("assets/config/game.toml");

    drone::GameLogger logger;
    logger.info("Juego iniciado (v" DRONE_VERSION ")");

    // Solo se construye el frontend elegido: los constructores tienen efectos
    // colaterales (raw mode de la terminal, apertura de la ventana GL) que no
    // deben ocurrir en el modo que no se está usando.
    std::unique_ptr<drone::IInputSource> input;
    std::unique_ptr<drone::IRenderer> renderer;
#ifdef DRONE_HAS_RAYLIB
    // Compartido entre entrada y renderizador, y declarado antes que ellos
    // para que les sobreviva: la camara libre se queda con W/A/S/D y la
    // entrada tiene que saberlo para no pilotar el dron a la vez.
    drone::RaylibViewState viewState;
    if (useGui) {
        input = std::make_unique<drone::RaylibInput>(viewState);
        renderer = std::make_unique<drone::RaylibRenderer>(viewState);
    }
#endif
    if (!input) {
        input = std::make_unique<drone::TerminalInput>();
        renderer = std::make_unique<drone::TerminalRenderer>();
    }

    drone::GameController game(*input, *renderer, config, physCfg);
    logger.attach(game.getEventBus());

    if (auto old = drone::loadGame(drone::saveFilePath()); old.version > 0) {
        game.applyLoad(old);
        logger.info("Partida guardada cargada");
    }

    using clock = std::chrono::steady_clock;
    auto last = clock::now();

    // Sin tope el bucle giraba a ~650 FPS: 11 redibujados por paso de fisica y
    // 344 KB/s de secuencias ANSI a la terminal, para un HUD que solo puede
    // cambiar a 60 Hz. Se limita al ritmo del paso fijo.
    const auto frameBudget = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<float>(config.fixedTimestep));
    auto nextFrame = clock::now();

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

        nextFrame += frameBudget;
        const auto after = clock::now();
        if (after < nextFrame) {
            std::this_thread::sleep_for(nextFrame - after);
        } else {
            // Se ha ido el presupuesto (pausa del SO, terminal lenta): se
            // reancla en vez de acumular deuda y correr para recuperarla.
            nextFrame = after;
        }
    }

    return 0;
}
