#pragma once

#include <memory>
#include <string>

#include "core/EventBus.h"
#include "core/Events.h"

namespace drone {

// Logger de juego que se suscribe al EventBus para registrar eventos del core
// y estadisticas periodicas. La implementacion spdlog vive aqui (app layer);
// el core jamas toca spdlog ni conoce este modulo (PLAN3.md P0-1, 14.2.1).
class GameLogger {
public:
    GameLogger();
    ~GameLogger();

    GameLogger(const GameLogger&) = delete;
    GameLogger& operator=(const GameLogger&) = delete;

    // Conecta este logger al bus de eventos de la simulacion.
    void attach(EventBus& bus);

    // Registra una entrada informativa fuera del ciclo de eventos
    // (p.ej. inicio/fin del programa, carga de configuracion).
    void info(const std::string& msg);
    void warn(const std::string& msg);

private:
    void onEvent(const Event& e);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace drone
