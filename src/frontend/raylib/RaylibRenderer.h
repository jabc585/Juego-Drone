#pragma once

#include <raylib.h>

#include <vector>

#include "frontend/IRenderer.h"
#include "frontend/raylib/ParticleSystem.h"
#include "frontend/raylib/RaylibViewState.h"

namespace drone {

class RaylibRenderer : public IRenderer {
public:
    explicit RaylibRenderer(RaylibViewState& view);
    ~RaylibRenderer() override;

    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;

    void draw(const WorldState& state, float alpha) override;
    void onEvent(const Event& event) override;

private:
    void drawBackground();
    void drawGround();
    void drawLandingZones(const WorldState& state);
    void drawDrone(const WorldState& state, float alpha);
    void drawObstacles(const WorldState& state);
    void drawHUD(const WorldState& state);
    void drawMinimap(const WorldState& state);
    void drawAttitudeIndicator(const WorldState& state);
    void drawBatteryVignette(const WorldState& state);
    void drawCompass(const WorldState& state);
    void drawStateOverlay(const WorldState& state);

    void updateCamera(const WorldState& state);
    void updateCameraFollow(const WorldState& state);
    void updateCameraFree();
    void updateCameraOrbit(const WorldState& state);
    void updateCursor(const WorldState& state);
    void ensureRenderTargets();
    void cycleCameraMode();
    void handleInput();

    Camera3D* m_camera = nullptr;
    Camera3D* m_freeCamera = nullptr;
    Shader m_lightShader = {};
    int m_shaderLightDir = 0, m_shaderLightCol = 0, m_shaderAmbient = 0;
    int m_shaderViewPos = 0, m_shaderFog = 0;
    // El modo vive en el estado compartido: la entrada tiene que consultarlo
    // para no mandar al dron las teclas de la camara libre.
    RaylibViewState& m_view;
    bool m_cursorHidden = true;
    // Estela: posiciones recientes del dron mientras va rapido.
    std::vector<Vector3> m_trail;

    // Desenfoque de movimiento: la escena se pinta en una textura y se mezcla
    // con la del frame anterior. El HUD va aparte, ya sobre la pantalla, para
    // que el texto no arrastre.
    RenderTexture2D m_sceneRT = {};
    RenderTexture2D m_historyRT = {};
    bool m_targetsReady = false;
    float m_orbitRadius = 20.0f;
    float m_orbitAngleH = 0.0f;
    float m_orbitAngleV = 0.5f;
    bool m_cameraOrbitNeedsReset = true;

    int m_screenW = 1280, m_screenH = 720;
    float m_messageTimer = 0.0f;
    char m_message[128] = {};
    int m_frameCount = 0;
    ParticleSystem m_dustParticles;
    ParticleSystem m_sparkParticles;
    bool m_wasGrounded = true;
    // Ultima posicion dibujada del dron. onEvent no recibe el WorldState, y
    // sin esto las chispas de choque salian en el origen del mapa.
    Vector3 m_lastDronePos = {0.0f, 0.0f, 0.0f};
};

}  // namespace drone
