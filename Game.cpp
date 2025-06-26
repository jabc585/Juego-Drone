#include "Game.h"
#include <iostream>
#include <chrono>
#include <thread>

Game::Game() : isRunning(false), isPaused(false) {}

void Game::init() {
    std::cout << "Inicializando el juego..." << std::endl;
    environment.loadEnvironment("Ciudad Futurista");
    // Configuración inicial de progresión del jugador
    playerProgression.displayProgress();
    isRunning = true;
}

void Game::processInput() {
    // Ejemplo simplificado de entrada.
    // Para un juego real se recomienda el uso de SDL2 o SFML para eventos asíncronos.
    char input;
    std::cout << "Comandos: [WASD] movimiento, [Q/E] ascender/descender, [P] pausa: ";
    std::cin >> input;
    
    if (input == 'P' || input == 'p') {
        togglePause();
        return;
    }
    
    if (!isPaused) {
        switch(input) {
            case 'W': case 'w':
                drone.move(0, 0, 1);
                break;
            case 'S': case 's':
                drone.move(0, 0, -1);
                break;
            case 'A': case 'a':
                drone.move(-1, 0, 0);
                break;
            case 'D': case 'd':
                drone.move(1, 0, 0);
                break;
            case 'Q': case 'q':
                drone.move(0, 1, 0);
                break;
            case 'E': case 'e':
                drone.move(0, -1, 0);
                break;
            default:
                break;
        }
    }
}

void Game::update(float deltaTime) {
    if (!isPaused) {
        // Actualiza la física del dron, incluyendo efectos de viento
        drone.updatePhysics(deltaTime);
        physicsEngine.update(deltaTime);
        // Ejemplo de progresión: ganar experiencia con cada actualización
        playerProgression.addExperience(5);
        // Incrementa la dificultad en el entorno gradualmente
        environment.increaseDifficulty();
    }
}

void Game::render() {
    // Renderiza entorno, UI y estado del dron
    environment.render();
    uiManager.renderUI();
    drone.displayStatus();
    // Muestra la progresión del jugador
    playerProgression.displayProgress();
}

void Game::togglePause() {
    isPaused = !isPaused;
    if (isPaused) {
        std::cout << "Juego en pausa." << std::endl;
        uiManager.showPauseMenu();
        // En pausa, podrías también permitir acceder a la configuración
        char choice;
        std::cout << "¿Acceder a configuración? (s/n): ";
        std::cin >> choice;
        if(choice == 's' || choice == 'S') {
            uiManager.showSettingsMenu();
        }
    } else {
        std::cout << "Reanudando juego..." << std::endl;
    }
}

void Game::run() {
    init();
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();

    while(isRunning) {
        auto currentTime = clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        float deltaTime = elapsed.count();
        lastTime = currentTime;

        processInput();
        update(deltaTime);
        render();

        // Simulación de frame rate ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // Estrategia básica para depuración: preguntar si se desea continuar
        char exitChoice;
        std::cout << "¿Continuar? (s/n): ";
        std::cin >> exitChoice;
        if(exitChoice == 'n' || exitChoice == 'N') {
            isRunning = false;
        }
    }
    shutdown();
}

void Game::shutdown() {
    std::cout << "Cerrando el juego..." << std::endl;
    // Aquí se liberarían recursos, guardar estados, etc.
}

