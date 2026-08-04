#pragma once

#include <chrono>
#include <string>

#include "frontend/IRenderer.h"

namespace drone {

// HUD de terminal repintado en sitio con códigos ANSI (PLAN2.md §14.1).
// Si stdout no es una TTY (tests de humo, pipes) emite un estado compacto
// por línea a intervalos, sin códigos de control.
class TerminalRenderer : public IRenderer {
public:
    TerminalRenderer();
    ~TerminalRenderer() override;

    TerminalRenderer(const TerminalRenderer&) = delete;
    TerminalRenderer& operator=(const TerminalRenderer&) = delete;

    void draw(const WorldState& state, float alpha) override;
    void onEvent(const Event& event) override;

private:
    using Clock = std::chrono::steady_clock;

    std::string buildFrame(const WorldState& state) const;
    void updateFps();

    bool m_isTty = false;
    std::string m_message;
    Clock::time_point m_messageUntil{};
    Clock::time_point m_lastPlainPrint{};
    Clock::time_point m_fpsWindowStart{};
    int m_framesInWindow = 0;
    float m_fps = 0.0f;
};

}  // namespace drone
