#include "frontend/raylib/RaylibRenderer.h"

#include <raylib.h>
#include <rlgl.h>

#include <cmath>
#include <cstdio>

#include "core/Events.h"

namespace drone {

namespace {

constexpr float kCameraBack = 18.0f;
constexpr float kCameraUp = 9.0f;
constexpr float kCameraLerp = 3.0f;
constexpr float kWorldSize = 200.0f;

constexpr Color kSkyTop = {50, 70, 120, 255};
constexpr Color kSkyBot = {140, 170, 220, 255};
constexpr Color kGroundColor = {45, 100, 50, 255};
constexpr Color kGridColor = {60, 130, 60, 255};
constexpr Color kDroneColor = {200, 50, 40, 255};
constexpr Color kArmColor = {160, 160, 170, 255};
constexpr Color kMotorColor = {50, 50, 60, 255};
constexpr Color kPropColor = {200, 200, 210, 140};
constexpr Color kObstacleColor = {100, 100, 120, 255};
constexpr Color kObstacleWire = {180, 180, 200, 255};
constexpr Color kTrunkColor = {104, 74, 48, 255};
constexpr Color kCanopyColor = {58, 132, 62, 255};
constexpr Color kRockColor = {126, 112, 96, 255};
constexpr Color kZoneColor = {255, 215, 0, 80};
constexpr Color kZoneWire = {255, 200, 0, 200};
constexpr Color kHudColor = {220, 220, 220, 255};
constexpr Color kWarnColor = {255, 200, 50, 255};
constexpr Color kOverlayBg = {0, 0, 0, 170};

const float kZoneRadius = 2.0f;

// Estela: por debajo de esta velocidad no se dibuja, y este es su largo.
constexpr float kTrailMinSpeed = 5.0f;
constexpr std::size_t kTrailPoints = 40;

// Desenfoque de movimiento: por debajo de esta velocidad no se aplica, y este
// es el peso maximo que conserva el frame anterior.
constexpr float kBlurMinSpeed = 15.0f;
constexpr float kBlurMaxHistory = 0.55f;

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
            return "[1/P] Reanudar    [2/F12] Camara    [3/Esc] Salir";
        case GameState::Settings:
            return "F12: Follow/Free/Orbit · Rueda: zoom · ClickDer: orbita";
        case GameState::GameOver:
            return "[R/1] Reiniciar    [3/Esc] Salir";
        default:
            return "";
    }
}

void drawCentered(const char* text, int y, int size, Color color) {
    int sw = GetScreenWidth();
    DrawText(text, (sw - MeasureText(text, size)) / 2, y, size, color);
}

const char* cameraModeName(CameraMode m) {
    switch (m) {
        case CameraMode::Follow:
            return "FOLLOW";
        case CameraMode::Free:
            return "FREE";
        case CameraMode::Orbit:
            return "ORBIT";
    }
    return "?";
}

}  // namespace

RaylibRenderer::RaylibRenderer(RaylibViewState& view) : m_view(view) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(m_screenW, m_screenH, "DroneFlightSim");
    DisableCursor();
    SetTargetFPS(60);

    m_camera = new Camera3D{};
    m_camera->position = {0.0f, kCameraUp, kCameraBack};
    m_camera->target = {0.0f, 0.0f, 0.0f};
    m_camera->up = {0.0f, 1.0f, 0.0f};
    m_camera->fovy = 60.0f;
    m_camera->projection = CAMERA_PERSPECTIVE;

    m_freeCamera = new Camera3D{};
    m_freeCamera->position = {20.0f, 10.0f, 20.0f};
    m_freeCamera->target = {0.0f, 2.0f, 0.0f};
    m_freeCamera->up = {0.0f, 1.0f, 0.0f};
    m_freeCamera->fovy = 60.0f;
    m_freeCamera->projection = CAMERA_PERSPECTIVE;

    m_frameCount = 0;

    // Shader de iluminacion
    Shader s = LoadShader("assets/shaders/lighting.vs", "assets/shaders/lighting.fs");
    m_lightShader = s;
    m_shaderLightDir = GetShaderLocation(s, "lightDir");
    m_shaderLightCol = GetShaderLocation(s, "lightColor");
    m_shaderAmbient = GetShaderLocation(s, "ambientColor");
    m_shaderViewPos = GetShaderLocation(s, "viewPos");
    m_shaderFog = GetShaderLocation(s, "fogDensity");
}

RaylibRenderer::~RaylibRenderer() {
    if (m_lightShader.id)
        UnloadShader(m_lightShader);
    delete m_camera;
    delete m_freeCamera;
    if (m_targetsReady) {
        UnloadRenderTexture(m_sceneRT);
        UnloadRenderTexture(m_historyRT);
    }
    CloseWindow();
}

// Las texturas se crean al vuelo y se rehacen si cambia el tamano: una textura
// del tamano viejo estiraria la imagen al redimensionar la ventana.
void RaylibRenderer::ensureRenderTargets() {
    // En pantallas retina el framebuffer es mayor que el tamano logico. Si la
    // textura se crea con el logico, la escena se dibuja a media resolucion y
    // se estira: hay que usar el tamano de render.
    const int w = GetRenderWidth();
    const int h = GetRenderHeight();
    if (m_targetsReady && m_sceneRT.texture.width == w && m_sceneRT.texture.height == h)
        return;
    if (m_targetsReady) {
        UnloadRenderTexture(m_sceneRT);
        UnloadRenderTexture(m_historyRT);
    }
    m_sceneRT = LoadRenderTexture(w, h);
    m_historyRT = LoadRenderTexture(w, h);
    m_targetsReady = true;
}

void RaylibRenderer::draw(const WorldState& state, float alpha) {
    if (!IsWindowReady())
        return;
    ++m_frameCount;

    if (IsWindowResized()) {
        m_screenW = GetScreenWidth();
        m_screenH = GetScreenHeight();
    }

    handleInput();
    updateCamera(state);
    updateCursor(state);

    // Configurar shader de iluminacion
    Camera3D* activeCam = m_view.cameraMode == CameraMode::Free ? m_freeCamera : m_camera;
    float lightDir[3] = {0.5f, -1.0f, 0.3f};
    float lightCol[3] = {1.0f, 0.95f, 0.85f};
    float ambient[3] = {0.5f, 0.55f, 0.6f};
    float viewPos[3] = {activeCam->position.x, activeCam->position.y, activeCam->position.z};
    float fogDensity = 0.002f;
    SetShaderValue(m_lightShader, m_shaderLightDir, lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_lightShader, m_shaderLightCol, lightCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_lightShader, m_shaderAmbient, ambient, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_lightShader, m_shaderViewPos, viewPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_lightShader, m_shaderFog, &fogDensity, SHADER_UNIFORM_FLOAT);

    ensureRenderTargets();

    // La escena va a una textura; el HUD no, para que el texto no arrastre.
    BeginTextureMode(m_sceneRT);
    // Imprescindible: BeginTextureMode no limpia nada. Sin esto el buffer de
    // profundidad conserva el del frame anterior y casi toda la geometria se
    // descarta por el test de profundidad — el suelo desaparecia entero.
    ClearBackground(BLANK);
    drawBackground();
    BeginShaderMode(m_lightShader);
    BeginMode3D(*activeCam);

    drawGround();
    drawLandingZones(state);
    drawObstacles(state);
    drawDrone(state, alpha);

    // Particulas
    m_dustParticles.update(GetFrameTime());
    m_sparkParticles.update(GetFrameTime());
    m_dustParticles.draw();
    m_sparkParticles.draw();

    EndMode3D();
    EndShaderMode();
    EndTextureMode();

    // Acumulacion: historial = k*escena + (1-k)*historial. Con k = 1 no hay
    // arrastre, que es lo que toca por debajo del umbral de velocidad.
    const float speed = state.droneVelocity.length();
    float keep = 1.0f;
    if (speed > kBlurMinSpeed) {
        const float over = std::min(1.0f, (speed - kBlurMinSpeed) / kBlurMinSpeed);
        keep = 1.0f - kBlurMaxHistory * over;
    }
    // Las texturas de raylib salen invertidas en vertical: alto negativo.
    const float tw = static_cast<float>(m_sceneRT.texture.width);
    const float th = static_cast<float>(m_sceneRT.texture.height);
    const Rectangle src = {0.0f, 0.0f, tw, -th};
    BeginTextureMode(m_historyRT);
    DrawTextureRec(m_sceneRT.texture, src, {0.0f, 0.0f},
                   {255, 255, 255, static_cast<unsigned char>(255.0f * keep)});
    EndTextureMode();

    // A pantalla va escalada al tamano logico, que es en el que se posiciona
    // el HUD.
    const Rectangle dst = {0.0f, 0.0f, static_cast<float>(m_screenW),
                           static_cast<float>(m_screenH)};
    BeginDrawing();
    DrawTexturePro(m_historyRT.texture, src, dst, {0.0f, 0.0f}, 0.0f, WHITE);
    drawHUD(state);
    drawMinimap(state);
    drawAttitudeIndicator(state);
    drawBatteryVignette(state);
    drawCompass(state);
    drawStateOverlay(state);
    EndDrawing();
}

void RaylibRenderer::handleInput() {
    if (IsKeyPressed(KEY_F12))
        cycleCameraMode();
    if (IsKeyPressed(KEY_F11))
        ToggleFullscreen();
}

void RaylibRenderer::cycleCameraMode() {
    switch (m_view.cameraMode) {
        case CameraMode::Follow:
            m_view.cameraMode = CameraMode::Free;
            break;
        case CameraMode::Free:
            m_view.cameraMode = CameraMode::Orbit;
            m_cameraOrbitNeedsReset = true;
            break;
        case CameraMode::Orbit:
            m_view.cameraMode = CameraMode::Follow;
            break;
    }
}

// El raton se captura solo mientras se juega: en los menus hace falta verlo.
// La camara libre en primera persona lo necesita capturado para poder mirar.
void RaylibRenderer::updateCursor(const WorldState& state) {
    const bool hide = state.state == GameState::Playing;
    if (hide == m_cursorHidden)
        return;
    m_cursorHidden = hide;
    if (hide)
        DisableCursor();
    else
        EnableCursor();
}

void RaylibRenderer::updateCamera(const WorldState& state) {
    switch (m_view.cameraMode) {
        case CameraMode::Follow:
            updateCameraFollow(state);
            break;
        case CameraMode::Free:
            updateCameraFree();
            break;
        case CameraMode::Orbit:
            updateCameraOrbit(state);
            break;
    }
}

void RaylibRenderer::updateCameraFollow(const WorldState& state) {
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

void RaylibRenderer::updateCameraFree() {
    UpdateCamera(m_freeCamera, CAMERA_FIRST_PERSON);
}

void RaylibRenderer::updateCameraOrbit(const WorldState& state) {
    // El centro sigue al dron. Antes se heredaba el target que dejo la camara
    // de seguimiento y la orbita se quedaba girando donde estuvo el dron.
    m_camera->target = {state.dronePosition.x, state.dronePosition.y, state.dronePosition.z};

    if (m_cameraOrbitNeedsReset) {
        m_orbitRadius = 20.0f;
        m_orbitAngleH = 0.0f;
        m_orbitAngleV = 0.5f;
        m_cameraOrbitNeedsReset = false;
    }
    m_orbitRadius -= GetMouseWheelMove() * 2.0f;
    m_orbitRadius = std::max(5.0f, std::min(100.0f, m_orbitRadius));

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        m_orbitAngleH += GetMouseDelta().x * 0.005f;
        m_orbitAngleV += GetMouseDelta().y * 0.005f;
        m_orbitAngleV = std::max(-1.4f, std::min(1.4f, m_orbitAngleV));
    }

    m_camera->position.x =
        m_camera->target.x + m_orbitRadius * cosf(m_orbitAngleV) * sinf(m_orbitAngleH);
    m_camera->position.y = m_camera->target.y + m_orbitRadius * sinf(m_orbitAngleV);
    m_camera->position.z =
        m_camera->target.z + m_orbitRadius * cosf(m_orbitAngleV) * cosf(m_orbitAngleH);
}

void RaylibRenderer::drawBackground() {
    for (int i = 0; i < m_screenH; ++i) {
        float t = static_cast<float>(i) / m_screenH;
        Color c = {static_cast<unsigned char>(kSkyBot.r + (kSkyTop.r - kSkyBot.r) * t),
                   static_cast<unsigned char>(kSkyBot.g + (kSkyTop.g - kSkyBot.g) * t),
                   static_cast<unsigned char>(kSkyBot.b + (kSkyTop.b - kSkyBot.b) * t), 255};
        DrawRectangle(0, i, m_screenW, 1, c);
    }
}

void RaylibRenderer::drawGround() {
    const float hw = kWorldSize / 2;
    DrawPlane({0, -0.05f, 0}, {kWorldSize, kWorldSize}, kGroundColor);
    for (float i = -hw; i <= hw; i += 10.0f) {
        DrawLine3D({i, 0, -hw}, {i, 0, hw}, kGridColor);
        DrawLine3D({-hw, 0, i}, {hw, 0, i}, kGridColor);
    }
}

void RaylibRenderer::drawLandingZones(const WorldState& state) {
    for (const auto& z : state.landingZonePositions) {
        DrawCylinder({z.x, 0.01f, z.z}, kZoneRadius, kZoneRadius, 0.05f, 12, kZoneColor);
        DrawCylinderWires({z.x, 0.01f, z.z}, kZoneRadius, kZoneRadius, 0.05f, 12, kZoneWire);
        DrawCircle3D({z.x, 3.0f, z.z}, kZoneRadius * 0.8f, {0, 1, 0}, 90.0f, kZoneWire);
    }
}

void RaylibRenderer::drawDrone(const WorldState& state, float alpha) {
    const Vec3 p = state.dronePosition;
    const Vec3 v = state.droneVelocity;
    const float dt = alpha * (1.0f / 60.0f);
    const float x = p.x + v.x * dt, y = p.y + v.y * dt, z = p.z + v.z * dt;
    m_lastDronePos = {x, y, z};

    // Sombra proyectada. Antes era un aro de radio fijo dibujado a ras de
    // suelo: no cambiaba con la altura, asi que a 40 m seguia igual de grande
    // y no daba ninguna pista de a que altura estabas. Ahora se ensancha y se
    // desvanece con la distancia al suelo, que es lo que hace una sombra.
    {
        const float h = std::max(0.0f, y);
        const float spread = 1.0f + h * 0.06f;
        const float radius = 0.42f * spread;
        const float fade = 1.0f / (1.0f + h * 0.09f);
        const unsigned char shade = static_cast<unsigned char>(110.0f * fade);
        if (shade > 3) {
            DrawCylinder({x, 0.012f, z}, radius, radius, 0.004f, 20, {0, 0, 0, shade});
            DrawCircle3D({x, 0.016f, z}, radius, {0, 1, 0}, 90.0f,
                         {0, 0, 0, static_cast<unsigned char>(shade / 2)});
        }
    }

    const float qx = state.droneQx, qy = state.droneQy, qz = state.droneQz, qw = state.droneQw;
    const float xx = qx * qx, yy = qy * qy, zz = qz * qz;
    const float xy = qx * qy, xz = qx * qz, yz = qy * qz;
    const float wx = qw * qx, wy = qw * qy, wz = qw * qz;
    Matrix rot = {1 - 2 * (yy + zz),
                  2 * (xy + wz),
                  2 * (xz - wy),
                  0,
                  2 * (xy - wz),
                  1 - 2 * (xx + zz),
                  2 * (yz + wx),
                  0,
                  2 * (xz + wy),
                  2 * (yz - wx),
                  1 - 2 * (xx + yy),
                  0,
                  0,
                  0,
                  0,
                  1};

    auto world = [&](float lx, float ly, float lz) -> Vector3 {
        return {x + rot.m0 * lx + rot.m4 * ly + rot.m8 * lz,
                y + rot.m1 * lx + rot.m5 * ly + rot.m9 * lz,
                z + rot.m2 * lx + rot.m6 * ly + rot.m10 * lz};
    };

    const float armLen = 0.18f, armW = 0.03f, armT = 0.015f;
    const float d = armLen * 0.7071f;
    const float motorS = 0.04f, propR = 0.06f;

    DrawCube(world(0, 0, 0), 0.10f, 0.04f, 0.10f, kDroneColor);
    DrawCube(world(d, 0, d), armLen, armT, armW, kArmColor);
    DrawCube(world(-d, 0, d), armLen, armT, armW, kArmColor);
    DrawCube(world(-d, 0, -d), armLen, armT, armW, kArmColor);
    DrawCube(world(d, 0, -d), armLen, armT, armW, kArmColor);

    DrawCube(world(d, 0.025f, d), motorS, motorS, motorS, kMotorColor);
    DrawCube(world(-d, 0.025f, d), motorS, motorS, motorS, kMotorColor);
    DrawCube(world(-d, 0.025f, -d), motorS, motorS, motorS, kMotorColor);
    DrawCube(world(d, 0.025f, -d), motorS, motorS, motorS, kMotorColor);

    const float propAngle = m_frameCount * 0.5f;
    for (int i = 0; i < 4; ++i) {
        float sx = (i == 0 || i == 3 ? d : -d);
        float sz = (i < 2 ? d : -d);
        Vector3 propPos = world(sx, 0.045f, sz);
        rlPushMatrix();
        rlTranslatef(propPos.x, propPos.y, propPos.z);
        rlRotatef(propAngle * RAD2DEG + i * 90.0f, 0, 1, 0);
        DrawCylinder({0, 0, 0}, propR, propR, 0.003f, 16, kPropColor);
        rlPopMatrix();

        // Brillo del motor, ahora si proporcional al empuje: antes el
        // comentario lo prometia pero se dibujaba un punto de color fijo.
        const float t = state.motorThrust[i];
        if (t > 0.02f) {
            const unsigned char a = static_cast<unsigned char>(60.0f + 195.0f * t);
            DrawSphere(propPos, 0.012f + 0.030f * t, {255, 210, 120, a});
            DrawSphereWires(propPos, 0.020f + 0.055f * t, 4, 6,
                            {255, 170, 60, static_cast<unsigned char>(a / 3)});
        }
    }

    // Estela: solo por encima de la velocidad a la que el movimiento se lee
    // como "rapido"; por debajo ensuciaba la pantalla en vuelo estacionario.
    const float speed = v.length();
    if (speed > kTrailMinSpeed) {
        m_trail.push_back({x, y, z});
        if (m_trail.size() > kTrailPoints)
            m_trail.erase(m_trail.begin());
    } else if (!m_trail.empty()) {
        m_trail.erase(m_trail.begin());
    }
    for (std::size_t i = 1; i < m_trail.size(); ++i) {
        const float f = static_cast<float>(i) / static_cast<float>(m_trail.size());
        DrawLine3D(m_trail[i - 1], m_trail[i],
                   {200, 225, 255, static_cast<unsigned char>(110.0f * f)});
    }

    // Polvo al despegar/aterrizar
    bool grounded = state.dronePosition.y < 0.5f && state.droneVelocity.length() < 0.5f;
    if (m_wasGrounded && !grounded) {
        m_dustParticles.emitRing({x, 0.02f, z}, 15, 0.5f, 2.0f, 1.5f, 0.04f, {180, 150, 100, 255});
    }
    if (!m_wasGrounded && grounded) {
        m_dustParticles.emitRing({x, 0.02f, z}, 8, 0.3f, 1.0f, 1.0f, 0.03f, {180, 150, 100, 255});
    }
    m_wasGrounded = grounded;
}

void RaylibRenderer::drawObstacles(const WorldState& state) {
    for (const Obstacle& o : state.obstacles) {
        const Vector3 c = {o.center.x, o.center.y, o.center.z};
        Color fill = kObstacleColor;
        switch (o.kind) {
            case ObstacleKind::Trunk:
                fill = kTrunkColor;
                break;
            case ObstacleKind::Canopy:
                fill = kCanopyColor;
                break;
            case ObstacleKind::Rock:
                fill = kRockColor;
                break;
            case ObstacleKind::Building:
                break;
        }
        DrawCube(c, o.size.x, o.size.y, o.size.z, fill);
        // Las aristas claras destacan un edificio contra el cielo, pero en un
        // bosque cerrado convierten las copas en una marana de lineas.
        if (o.kind == ObstacleKind::Building)
            DrawCubeWires(c, o.size.x, o.size.y, o.size.z, kObstacleWire);
    }
}

void RaylibRenderer::drawHUD(const WorldState& state) {
    char buf[128];
    int sw = m_screenW, sh = m_screenH;

    // Panel HUD izquierdo
    DrawRectangle(5, 5, 200, 130, {0, 0, 0, 140});
    DrawRectangleLines(5, 5, 200, 130, {100, 100, 120, 100});

    std::snprintf(buf, sizeof(buf), "Bat: %.0f%%", static_cast<double>(state.battery));
    DrawText(buf, 15, 12, 18, state.battery > 20.0f ? kHudColor : kWarnColor);

    float alt = state.groundDistance > 0 ? state.groundDistance : state.dronePosition.y;
    std::snprintf(buf, sizeof(buf), "Alt: %.1f m", static_cast<double>(alt));
    DrawText(buf, 15, 35, 18, kHudColor);

    std::snprintf(buf, sizeof(buf), "Vel: %.1f m/s",
                  static_cast<double>(state.droneVelocity.length()));
    DrawText(buf, 15, 58, 18, kHudColor);

    const float wm = state.wind.length();
    const char* arrow = std::fabs(state.wind.x) >= std::fabs(state.wind.z)
                            ? (state.wind.x >= 0 ? ">" : "<")
                            : (state.wind.z >= 0 ? "v" : "^");
    std::snprintf(buf, sizeof(buf), "Viento: %s %.1f", wm > 0.1f ? arrow : "-",
                  static_cast<double>(wm));
    DrawText(buf, 15, 81, 18, kHudColor);

    std::snprintf(buf, sizeof(buf), "Dif: %.1f", static_cast<double>(state.difficulty));
    DrawText(buf, 15, 104, 18, kHudColor);

    // Zona mas cercana
    if (state.nearestZoneDist >= 0) {
        Color zc = state.nearestZoneDist < kZoneRadius + 1.0f ? kWarnColor : kHudColor;
        std::snprintf(buf, sizeof(buf), "Zona: %.0f m", static_cast<double>(state.nearestZoneDist));
        DrawText(buf, sw - 120, sh - 50, 18, zc);
    }

    // Modo de cámara
    std::snprintf(buf, sizeof(buf), "Cam: %s", cameraModeName(m_view.cameraMode));
    DrawText(buf, sw - 120, 10, 18, kHudColor);

    // Nivel + XP
    std::snprintf(buf, sizeof(buf), "Nivel %d  XP %d/%d", state.level, state.experience,
                  state.experienceToNext);
    DrawText(buf, 10, sh - 25, 18, kHudColor);

    // FPS
    DrawFPS(sw - 90, 10);

    // Mensaje de evento
    m_messageTimer -= GetFrameTime();
    if (m_messageTimer > 0.0f && m_message[0] != '\0') {
        DrawRectangle(0, sh - 100, sw, 30, {0, 0, 0, 150});
        drawCentered(m_message, sh - 95, 20, kWarnColor);
    }
}

void RaylibRenderer::drawMinimap(const WorldState& state) {
    constexpr int mmW = 150, mmH = 150;
    const float scale = mmW / kWorldSize;

    int mmX = m_screenW - mmW - 10;
    int mmY = m_screenH - mmH - 10;

    auto toMM = [&](float wx, float wz) -> Vector2 {
        return {mmX + mmW / 2.0f + wx * scale, mmY + mmH / 2.0f - wz * scale};
    };

    DrawRectangle(mmX, mmY, mmW, mmH, {0, 0, 0, 140});
    DrawRectangleLines(mmX, mmY, mmW, mmH, {100, 100, 120, 100});

    // Obstaculos
    for (const auto& o : state.obstacles) {
        Vector2 p = toMM(o.center.x, o.center.z);
        float r = std::max(o.size.x, o.size.z) * scale * 0.5f;
        DrawCircleV(p, std::max(2.0f, r), {120, 120, 130, 180});
    }

    // Landing zones
    for (const auto& z : state.landingZonePositions) {
        Vector2 p = toMM(z.x, z.z);
        DrawCircleV(p, kZoneRadius * scale, {255, 200, 0, 100});
        DrawCircleLines(p.x, p.y, kZoneRadius * scale, {255, 200, 0, 200});
    }

    // Dron
    Vector2 dp = toMM(state.dronePosition.x, state.dronePosition.z);
    DrawCircleV(dp, 3, {255, 60, 40, 255});
    DrawCircleLines(dp.x, dp.y, 4, {255, 100, 80, 200});
}

void RaylibRenderer::drawAttitudeIndicator(const WorldState& state) {
    constexpr int aiW = 100, aiH = 100;
    int aiX = m_screenW / 2 - aiW / 2;
    int aiY = m_screenH - aiH - 15;
    int cx = aiX + aiW / 2, cy = aiY + aiH / 2;

    DrawRectangle(aiX, aiY, aiW, aiH, {0, 0, 0, 100});
    DrawRectangleLines(aiX, aiY, aiW, aiH, {100, 100, 120, 80});

    // Horizonte: linea que rota con roll y se desplaza con pitch.
    // WorldState los da YA en radianes; multiplicarlos otra vez por pi/180
    // dejaba el horizonte practicamente plano (20 grados de alabeo se
    // dibujaban como 0,35).
    const float rollRad = state.droneRoll;
    float pitchOffset = state.dronePitch * 57.2958f * 1.5f;  // 1,5 px por grado
    if (pitchOffset > aiH / 2 - 2)
        pitchOffset = aiH / 2 - 2;
    if (pitchOffset < -aiH / 2 + 2)
        pitchOffset = -aiH / 2 + 2;

    float halfW = aiW * 0.6f;
    float cosR = cosf(rollRad), sinR = sinf(rollRad);

    Vector2 left = {cx - halfW * cosR, cy + pitchOffset - halfW * sinR};
    Vector2 right = {cx + halfW * cosR, cy + pitchOffset + halfW * sinR};
    DrawLineEx(left, right, 2, {100, 200, 100, 255});

    // Indicador central (dron)
    DrawCircle(cx, cy, 4, {255, 60, 40, 255});
}

void RaylibRenderer::drawBatteryVignette(const WorldState& state) {
    // Solo jugando: encima del velo de pausa o de fin de partida dejaba el
    // texto de los menus practicamente ilegible.
    if (state.state != GameState::Playing || state.battery > 30.0f)
        return;
    float intensity = 1.0f - state.battery / 30.0f;  // 0→1 as battery drops 30→0
    if (intensity < 0)
        intensity = 0;
    if (intensity > 1)
        intensity = 1;
    // Cuatro bandas degradadas hacia los bordes. Un rectangulo liso sobre
    // toda la pantalla no es una vinieta: teñia tambien el centro, el HUD y
    // el mini-mapa, y con la bateria en las ultimas no se veia nada.
    const unsigned char alpha = static_cast<unsigned char>(intensity * 170.0f);
    const Color edge = {70, 0, 0, alpha};
    const Color clear = {70, 0, 0, 0};
    const int bandY = m_screenH / 5;
    const int bandX = m_screenW / 5;

    DrawRectangleGradientV(0, 0, m_screenW, bandY, edge, clear);
    DrawRectangleGradientV(0, m_screenH - bandY, m_screenW, bandY, clear, edge);
    DrawRectangleGradientH(0, 0, bandX, m_screenH, edge, clear);
    DrawRectangleGradientH(m_screenW - bandX, 0, bandX, m_screenH, clear, edge);
}

void RaylibRenderer::drawCompass(const WorldState& state) {
    constexpr int cW = 200, cH = 34;
    int cX = m_screenW / 2 - cW / 2, cY = 5;

    DrawRectangle(cX, cY, cW, cH, {0, 0, 0, 120});
    DrawRectangleLines(cX, cY, cW, cH, {100, 100, 120, 80});

    float headingDeg = state.droneYaw * 57.2958f;
    while (headingDeg < 0.0f)
        headingDeg += 360.0f;
    while (headingDeg >= 360.0f)
        headingDeg -= 360.0f;

    // Antes ponia "%.0f N": una N fija junto al rumbo, que ademas se pisaba
    // con la marca cardinal movil. El rumbo va debajo y solo el numero.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.0f", static_cast<double>(headingDeg));
    int tw = MeasureText(buf, 14);
    DrawText(buf, cX + cW / 2 - tw / 2, cY + 17, 14, {220, 220, 220, 255});

    // Marcas cardinales
    for (int d : {0, 90, 180, 270}) {
        float offset = (d - headingDeg) * (cW / 360.0f);
        int x = static_cast<int>(cX + cW / 2 + offset);
        if (x >= cX && x <= cX + cW) {
            const char* label = d == 0 ? "N" : d == 90 ? "E" : d == 180 ? "S" : "W";
            DrawText(label, x - MeasureText(label, 12) / 2, cY + 3, 12, {255, 200, 50, 255});
        }
    }
}

void RaylibRenderer::drawStateOverlay(const WorldState& state) {
    const char* title = stateTitle(state.state);
    if (title == nullptr)
        return;
    DrawRectangle(0, 0, m_screenW, m_screenH, kOverlayBg);
    drawCentered(title, m_screenH / 2 - 40, 48, kWarnColor);
    drawCentered(stateHint(state.state), m_screenH / 2 + 30, 20, kHudColor);
}

void RaylibRenderer::onEvent(const Event& event) {
    m_messageTimer = 3.0f;
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
            m_sparkParticles.emit(m_lastDronePos, 20, 5.0f, 8.0f, 0.5f, 0.02f, {255, 200, 50, 255});
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
        case EventType::LandingZone:
            std::snprintf(m_message, sizeof(m_message), "Aterrizaje limpio: +%.0f XP",
                          static_cast<double>(event.value));
            break;
    }
}

}  // namespace drone
