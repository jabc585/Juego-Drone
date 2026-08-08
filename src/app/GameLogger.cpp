#include "app/GameLogger.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <cstdio>

namespace drone {

struct GameLogger::Impl {
    std::shared_ptr<spdlog::logger> logger;
};

GameLogger::GameLogger() : m_impl(std::make_unique<Impl>()) {
    try {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("drone_game.log",
                                                                           1024 * 1024 * 5, 3);
        m_impl->logger = std::make_shared<spdlog::logger>("drone", std::move(sink));
        m_impl->logger->set_level(spdlog::level::info);
        m_impl->logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        m_impl->logger->flush_on(spdlog::level::info);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[logger] no se pudo inicializar spdlog: %s\n", e.what());
    }
}

GameLogger::~GameLogger() {
    if (m_impl && m_impl->logger) {
        m_impl->logger->info("Juego cerrado");
        m_impl->logger->flush();
    }
}

void GameLogger::attach(EventBus& bus) {
    bus.subscribeAll([this](const Event& e) { onEvent(e); });
}

void GameLogger::onEvent(const Event& e) {
    if (!m_impl || !m_impl->logger)
        return;
    switch (e.type) {
        case EventType::BatteryLow:
            m_impl->logger->warn("Bateria baja: {:.1f}%", e.value);
            break;
        case EventType::BatteryEmpty:
            m_impl->logger->warn("Bateria agotada");
            break;
        case EventType::Collision:
            m_impl->logger->info("Colision a {:.1f} m/s", e.value);
            break;
        case EventType::LevelUp:
            m_impl->logger->info("Nivel {:.0f} alcanzado", e.value);
            break;
        case EventType::DroneUnlocked:
            m_impl->logger->info("Dron desbloqueado (nivel {:.0f})", e.value);
            break;
        case EventType::GameSaved:
            m_impl->logger->info("Partida guardada");
            break;
        case EventType::GameLoaded:
            m_impl->logger->info("Partida cargada");
            break;
        case EventType::LandingZone:
            m_impl->logger->info("Aterrizaje en zona: +{:.0f} XP", e.value);
            break;
    }
}

void GameLogger::info(const std::string& msg) {
    if (m_impl && m_impl->logger)
        m_impl->logger->info(msg);
}

void GameLogger::warn(const std::string& msg) {
    if (m_impl && m_impl->logger)
        m_impl->logger->warn(msg);
}

}  // namespace drone
