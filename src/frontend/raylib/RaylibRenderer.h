#pragma once

#include "frontend/IRenderer.h"

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
    struct Impl;
    Impl* m_impl;
};

}  // namespace drone
