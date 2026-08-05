#pragma once

#include "frontend/IRenderer.h"

// Se declara adelantada para no filtrar raylib.h a los consumidores.
struct Camera3D;

namespace drone {

class RaylibRenderer : public IRenderer {
public:
    RaylibRenderer();
    ~RaylibRenderer() override;

    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;

    void draw(const WorldState& state, float alpha) override;
    void onEvent(const Event& event) override;

private:
    void updateCamera(const WorldState& state);
    void drawDrone(const WorldState& state, float alpha);
    void drawObstacles(const WorldState& state);
    void drawHUD(const WorldState& state);
    void drawStateOverlay(const WorldState& state);

    Camera3D* m_camera = nullptr;
    float m_messageTimer = 0.0f;
    char m_message[128] = {};
};

}  // namespace drone
