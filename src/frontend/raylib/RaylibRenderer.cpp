#include "frontend/raylib/RaylibRenderer.h"

#include <raylib.h>

#include <cmath>
#include <cstdio>

#include "core/Events.h"

namespace drone {

namespace {

constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;
constexpr float kDroneRadius = 0.4f;
constexpr float kMessageSeconds = 3.0f;
// Desplazamiento de la cámara respecto al dron, en metros.
constexpr float kCameraBack = 18.0f;
constexpr float kCameraUp = 9.0f;
constexpr float kCameraLerp = 3.0f;  // 1/s — suavizado del seguimiento

constexpr Color kDroneColor = {180, 50, 50, 255};
constexpr Color kDroneWire = {255, 80, 80, 255};
constexpr Color kObstacleColor = {100, 100, 120, 255};
constexpr Color kObstacleWire = {180, 180, 200, 255};
constexpr Color kHudColor = {220, 220, 220, 255};
constexpr Color kWarnColor = {255, 200, 50, 255};
constexpr Color kOverlayBg = {0, 0, 0, 170};

const char* stateTitle(GameState s) {
    switch (s) {
        case GameState::Paused:
            return "PAUSA";
        case GameState::Settings:
            return "CONFIGURACION";
        case GameState::GameOver:
            return "FIN DE PARTIDA";
        default:
            return nullptr;
    }
}

const char* stateHint(GameState s) {
    switch (s) {
        case GameState::Paused:
            return "[1/P] Reanudar    [2] Configuracion    [3/Esc] Salir";
        case GameState::Settings:
            return "Controles: WASD/flechas + Q/E.  Cualquier tecla para volver.";
        case GameState::GameOver:
            return "[R/1] Reiniciar    [3/Esc] Salir";
        default:
            return "";
    }
}

void drawCentered(const char* text, int y, int size, Color color) {
    DrawText(text, (kScreenW - MeasureText(text, size)) / 2, y, size, color);
}

}  // namespace

RaylibRenderer::RaylibRenderer() {
    // Sin esto raylib escupe ~30 líneas de INFO al arrancar.
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(kScreenW, kScreenH, "DroneFlightSim");
    SetTargetFPS(60);

    m_camera = new Camera3D{};
    m_camera->position = {0.0f, kCameraUp, kCameraBack};
    m_camera->target = {0.0f, 0.0f, 0.0f};
    m_camera->up = {0.0f, 1.0f, 0.0f};
    m_camera->fovy = 60.0f;
    m_camera->projection = CAMERA_PERSPECTIVE;
}

RaylibRenderer::~RaylibRenderer() {
    delete m_camera;
    CloseWindow();
}

void RaylibRenderer::draw(const WorldState& state, float alpha) {
    if (!IsWindowReady())
        return;

    updateCamera(state);

    BeginDrawing();
    ClearBackground({10, 10, 30, 255});

    BeginMode3D(*m_camera);
    DrawGrid(40, 5.0f);
    drawObstacles(state);
    drawDrone(state, alpha);
    EndMode3D();

    drawHUD(state);
    drawStateOverlay(state);
    EndDrawing();
}

void RaylibRenderer::updateCamera(const WorldState& state) {
    // La cámara sigue al dron: sin esto se pierde de vista en cuanto se aleja
    // del origen (el mundo mide 200 m de lado).
    const Vec3 p = state.dronePosition;
    const Vector3 desired = {p.x, p.y + kCameraUp, p.z + kCameraBack};
    const float t = std::fmin(1.0f, kCameraLerp * GetFrameTime());

    m_camera->position.x += (desired.x - m_camera->position.x) * t;
    m_camera->position.y += (desired.y - m_camera->position.y) * t;
    m_camera->position.z += (desired.z - m_camera->position.z) * t;

    m_camera->target.x += (p.x - m_camera->target.x) * t;
    m_camera->target.y += (p.y - m_camera->target.y) * t;
    m_camera->target.z += (p.z - m_camera->target.z) * t;
}

void RaylibRenderer::drawDrone(const WorldState& state, float alpha) {
    const Vec3 p = state.dronePosition;
    const Vec3 v = state.droneVelocity;

    // Interpola el resto de step pendiente para que el movimiento no se vea
    // escalonado cuando el render va más rápido que la simulación.
    const float dt = alpha * (1.0f / 60.0f);
    const Vector3 pos = {p.x + v.x * dt, p.y + v.y * dt, p.z + v.z * dt};

    // Sombra proyectada: da sensación de altura, que en 3D es difícil de leer.
    DrawCircle3D({pos.x, 0.02f, pos.z}, kDroneRadius * 2.0f, {1.0f, 0.0f, 0.0f}, 90.0f,
                 {0, 0, 0, 120});
    DrawSphere(pos, kDroneRadius, kDroneColor);
    DrawSphereWires(pos, kDroneRadius, 6, 8, kDroneWire);
}

void RaylibRenderer::drawObstacles(const WorldState& state) {
    for (const Obstacle& o : state.obstacles) {
        const Vector3 c = {o.center.x, o.center.y, o.center.z};
        DrawCube(c, o.size.x, o.size.y, o.size.z, kObstacleColor);
        DrawCubeWires(c, o.size.x, o.size.y, o.size.z, kObstacleWire);
    }
}

void RaylibRenderer::drawHUD(const WorldState& state) {
    char buf[128];

    std::snprintf(buf, sizeof(buf), "Bateria: %.0f%%", static_cast<double>(state.battery));
    DrawText(buf, 10, 10, 20, state.battery > 20.0f ? kHudColor : kWarnColor);

    std::snprintf(buf, sizeof(buf), "Altitud: %.1f m", static_cast<double>(state.dronePosition.y));
    DrawText(buf, 10, 40, 20, kHudColor);

    std::snprintf(buf, sizeof(buf), "Velocidad: %.1f m/s",
                  static_cast<double>(state.droneVelocity.length()));
    DrawText(buf, 10, 70, 20, kHudColor);

    const float windMag = state.wind.length();
    const char* arrow = std::fabs(state.wind.x) >= std::fabs(state.wind.z)
                            ? (state.wind.x >= 0 ? ">" : "<")
                            : (state.wind.z >= 0 ? "v" : "^");
    std::snprintf(buf, sizeof(buf), "Viento: %s %.1f m/s   Dificultad: %.1f",
                  windMag > 0.1f ? arrow : "-", static_cast<double>(windMag),
                  static_cast<double>(state.difficulty));
    DrawText(buf, 10, 100, 20, kHudColor);

    std::snprintf(buf, sizeof(buf), "Nivel %d | XP: %d/%d", state.level, state.experience,
                  state.experienceToNext);
    DrawText(buf, 10, kScreenH - 30, 20, kHudColor);

    DrawText("WASD mover | Q/E subir/bajar | P pausa | F5/F9 guardar/cargar | Esc salir", 10,
             kScreenH - 55, 14, {150, 150, 160, 255});
    DrawFPS(kScreenW - 90, 10);

    m_messageTimer -= GetFrameTime();
    if (m_messageTimer > 0.0f && m_message[0] != '\0') {
        drawCentered(m_message, kScreenH - 90, 20, kWarnColor);
    }
}

void RaylibRenderer::drawStateOverlay(const WorldState& state) {
    const char* title = stateTitle(state.state);
    if (title == nullptr)
        return;

    // Sin esto, en modo gráfico no había forma de saber que el juego estaba
    // pausado o terminado: la escena simplemente se congelaba.
    DrawRectangle(0, 0, kScreenW, kScreenH, kOverlayBg);
    drawCentered(title, kScreenH / 2 - 40, 48, kWarnColor);
    drawCentered(stateHint(state.state), kScreenH / 2 + 30, 20, kHudColor);
}

void RaylibRenderer::onEvent(const Event& event) {
    m_messageTimer = kMessageSeconds;
    switch (event.type) {
        case EventType::BatteryLow:
            std::snprintf(m_message, sizeof(m_message), "Bateria baja (%.0f%%)!",
                          static_cast<double>(event.value));
            break;
        case EventType::BatteryEmpty:
            std::snprintf(m_message, sizeof(m_message), "Bateria agotada!");
            break;
        case EventType::Collision:
            std::snprintf(m_message, sizeof(m_message), "Colision a %.1f m/s!",
                          static_cast<double>(event.value));
            break;
        case EventType::LevelUp:
            std::snprintf(m_message, sizeof(m_message), "Nivel %.0f alcanzado!",
                          static_cast<double>(event.value));
            break;
        case EventType::DroneUnlocked:
            std::snprintf(m_message, sizeof(m_message), "Nuevo dron desbloqueado!");
            break;
        case EventType::GameSaved:
            std::snprintf(m_message, sizeof(m_message), "Partida guardada");
            break;
        case EventType::GameLoaded:
            std::snprintf(m_message, sizeof(m_message), "Partida cargada");
            break;
    }
}

}  // namespace drone
