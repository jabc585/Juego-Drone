#include "frontend/terminal/TerminalRenderer.h"

#include <cmath>
#include <cstdio>
#include <sstream>

#if defined(_WIN32)
#include <io.h>
#define DRONE_ISATTY(fd) (_isatty(fd) != 0)
#else
#include <unistd.h>
#define DRONE_ISATTY(fd) (isatty(fd) == 1)
#endif

namespace drone {

namespace {

constexpr int kBatteryBarWidth = 10;
constexpr std::chrono::seconds kMessageLifetime{3};
constexpr int kLineBufferSize = 160;

std::string batteryBar(float battery) {
    const int filled =
        static_cast<int>(std::round(battery / 100.0f * static_cast<float>(kBatteryBarWidth)));
    std::string bar = "[";
    for (int i = 0; i < kBatteryBarWidth; ++i)
        bar += (i < filled ? "#" : ".");
    bar += "]";
    return bar;
}

// Flecha de dirección dominante del viento en el plano XZ.
const char* windArrow(const Vec3& wind) {
    if (wind.length() < 0.05f)
        return "·";
    const bool xDominant = std::fabs(wind.x) >= std::fabs(wind.z);
    if (xDominant)
        return wind.x >= 0 ? "→" : "←";
    return wind.z >= 0 ? "↑" : "↓";
}

const char* stateLabel(GameState s) {
    switch (s) {
        case GameState::Booting:
            return "INICIANDO";
        case GameState::Playing:
            return "VOLANDO";
        case GameState::Paused:
            return "PAUSA";
        case GameState::Settings:
            return "CONFIGURACIÓN";
        case GameState::GameOver:
            return "FIN DE PARTIDA";
        case GameState::ShuttingDown:
            return "CERRANDO";
    }
    return "?";
}

}  // namespace

TerminalRenderer::TerminalRenderer() {
    m_isTty = DRONE_ISATTY(1);
    if (m_isTty) {
        std::fputs("\x1b[?25l\x1b[2J", stdout);  // ocultar cursor + limpiar
    }
    m_fpsWindowStart = Clock::now();
    m_lastPlainPrint = Clock::now() - std::chrono::seconds(10);
}

TerminalRenderer::~TerminalRenderer() {
    if (m_isTty) {
        std::fputs("\x1b[?25h\x1b[0m\n", stdout);  // restaurar cursor
        std::fflush(stdout);
    }
}

void TerminalRenderer::updateFps() {
    ++m_framesInWindow;
    const auto now = Clock::now();
    const float elapsed = std::chrono::duration<float>(now - m_fpsWindowStart).count();
    if (elapsed >= 0.5f) {
        m_fps = static_cast<float>(m_framesInWindow) / elapsed;
        m_framesInWindow = 0;
        m_fpsWindowStart = now;
    }
}

std::string TerminalRenderer::buildFrame(const WorldState& s) const {
    std::ostringstream out;
    char line[kLineBufferSize];

    out << "\x1b[H";  // cursor a origen; cada línea termina con \x1b[K
    std::snprintf(line, sizeof(line), " DRONE FLIGHT SIMULATOR      %-14s FPS: %3.0f",
                  stateLabel(s.state), static_cast<double>(m_fps));
    out << line << "\x1b[K\n";
    out << "-----------------------------------------------------\x1b[K\n";
    std::snprintf(line, sizeof(line), " Altitud:  %6.1f m    Velocidad: %5.1f m/s",
                  static_cast<double>(s.dronePosition.y),
                  static_cast<double>(s.droneVelocity.length()));
    out << line << "\x1b[K\n";
    std::snprintf(line, sizeof(line), " Batería:  %s %5.1f %%", batteryBar(s.battery).c_str(),
                  static_cast<double>(s.battery));
    out << line << "\x1b[K\n";
    std::snprintf(line, sizeof(line), " Viento:   %s %4.1f m/s    Dificultad: %.1f",
                  windArrow(s.wind), static_cast<double>(s.wind.length()),
                  static_cast<double>(s.difficulty));
    out << line << "\x1b[K\n";
    std::snprintf(line, sizeof(line), " Posición: (%.1f, %.1f, %.1f)",
                  static_cast<double>(s.dronePosition.x), static_cast<double>(s.dronePosition.y),
                  static_cast<double>(s.dronePosition.z));
    out << line << "\x1b[K\n";
    out << "-----------------------------------------------------\x1b[K\n";
    std::snprintf(line, sizeof(line), " Nivel %d   XP: %d/%d   Entorno: %s", s.level, s.experience,
                  s.experienceToNext, s.environmentName.c_str());
    out << line << "\x1b[K\n";

    switch (s.state) {
        case GameState::Paused:
            out << "\x1b[K\n === Menú de Pausa ===\x1b[K\n"
                   " [1/P] Reanudar  [2] Configuración  [3/X] Salir\x1b[K\n";
            break;
        case GameState::Settings:
            out << "\x1b[K\n === Configuración ===\x1b[K\n"
                   " Controles: WASD/flechas + Q/E. Cualquier tecla para volver."
                   "\x1b[K\n";
            break;
        case GameState::GameOver:
            out << "\x1b[K\n === FIN DE PARTIDA ===\x1b[K\n"
                   " [R/1] Reiniciar   [X/3] Salir\x1b[K\n";
            break;
        default:
            out << " Controles: WASD mover · Q/E subir/bajar · P pausa · F5/F9 "
                   "guardar/cargar · X/Esc salir\x1b[K\n\x1b[K\n\x1b[K\n";
            break;
    }

    const bool messageActive = Clock::now() < m_messageUntil;
    out << " " << (messageActive ? m_message : "") << "\x1b[K";
    return out.str();
}

void TerminalRenderer::draw(const WorldState& state, float /*alpha*/) {
    updateFps();

    if (m_isTty) {
        const std::string frame = buildFrame(state);
        std::fwrite(frame.data(), 1, frame.size(), stdout);
        std::fflush(stdout);
        return;
    }

    // Sin TTY: una línea de estado por segundo, apta para logs y humo.
    const auto now = Clock::now();
    if (now - m_lastPlainPrint < std::chrono::seconds(1))
        return;
    m_lastPlainPrint = now;
    std::printf("[%s] t=%.1fs alt=%.1fm bat=%.1f%% nivel=%d\n", stateLabel(state.state),
                static_cast<double>(state.simTime), static_cast<double>(state.dronePosition.y),
                static_cast<double>(state.battery), state.level);
    std::fflush(stdout);
}

void TerminalRenderer::onEvent(const Event& event) {
    char buffer[96];
    switch (event.type) {
        case EventType::BatteryLow:
            std::snprintf(buffer, sizeof(buffer),
                          "Batería baja (%.0f%%) — busca zona de aterrizaje",
                          static_cast<double>(event.value));
            break;
        case EventType::BatteryEmpty:
            std::snprintf(buffer, sizeof(buffer), "¡Batería agotada!");
            break;
        case EventType::Collision:
            std::snprintf(buffer, sizeof(buffer), "¡Colisión a %.1f m/s!",
                          static_cast<double>(event.value));
            break;
        case EventType::LevelUp:
            std::snprintf(buffer, sizeof(buffer), "¡Nivel %.0f alcanzado!",
                          static_cast<double>(event.value));
            break;
        case EventType::DroneUnlocked:
            std::snprintf(buffer, sizeof(buffer), "Nuevo dron desbloqueado: Modelo X avanzado");
            break;
        case EventType::GameSaved:
            std::snprintf(buffer, sizeof(buffer), "Partida guardada");
            break;
        case EventType::GameLoaded:
            std::snprintf(buffer, sizeof(buffer), "Partida cargada");
            break;
    }
    m_message = buffer;
    m_messageUntil = Clock::now() + kMessageLifetime;

    if (!m_isTty)
        std::printf("[EVENTO] %s\n", buffer);
}

}  // namespace drone
