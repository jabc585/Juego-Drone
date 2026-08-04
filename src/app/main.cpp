#include <cstdio>
#include <cstring>

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

    // Composición e inyección: aquí es el único sitio donde el frontend
    // concreto y el core se conocen.
    drone::TerminalInput input;
    drone::TerminalRenderer renderer;
    drone::GameController game(input, renderer);
    game.run();
    return 0;
}
