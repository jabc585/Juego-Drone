#include "core/GameController.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "core/Config.h"

namespace drone {

namespace {
constexpr int kAxisX = 0;
constexpr int kAxisY = 1;
constexpr int kAxisZ = 2;
// Cota de comandos procesados por frame para que una entrada a ráfagas
// (p. ej. un pipe) no monopolice el bucle.
constexpr int kMaxCommandsPerFrame = 32;
}  // namespace

GameController::GameController(IInputSource& input, IRenderer& renderer)
    : m_input(input), m_renderer(renderer), m_progression(&m_world.events()) {
    m_world.environment().loadEnvironment("Ciudad Futurista");

    m_world.events().subscribe(EventType::Collision, [this](const Event& e) {
        if (e.value > config::kCrashSpeed)
            m_crashed = true;
    });
    // El frontend recibe todos los eventos para mostrarlos como avisos.
    m_world.events().subscribeAll([this](const Event& e) { m_renderer.onEvent(e); });

    m_state = GameState::Playing;
}

void GameController::run() {
    using clock = std::chrono::steady_clock;
    auto last = clock::now();

    while (m_state != GameState::ShuttingDown) {
        const auto now = clock::now();
        const float frame = std::chrono::duration<float>(now - last).count();
        last = now;

        tick(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameController::tick(float frameSeconds) {
    // Clamp anti "espiral de la muerte" (PLAN2.md §10.3).
    m_accumulator += std::min(frameSeconds, config::kMaxFrameTime);

    for (int i = 0; i < kMaxCommandsPerFrame; ++i) {
        const Command cmd = m_input.poll();
        if (cmd == Command::None)
            break;
        handleCommand(cmd);
        if (m_state == GameState::ShuttingDown)
            return;
    }

    while (m_accumulator >= config::kFixedTimestep) {
        if (m_state == GameState::Playing)
            fixedUpdate(config::kFixedTimestep);
        m_accumulator -= config::kFixedTimestep;
    }

    m_renderer.draw(makeState(), m_accumulator / config::kFixedTimestep);
}

void GameController::handleCommand(Command cmd) {
    const auto pulse = [this](int axis, float dir) {
        m_pulseDir[axis] = dir;
        m_pulseTime[axis] = config::kThrustPulseSeconds;
    };

    switch (m_state) {
        case GameState::Playing:
            switch (cmd) {
                case Command::ThrustForward:
                    pulse(kAxisZ, 1.0f);
                    break;
                case Command::ThrustBackward:
                    pulse(kAxisZ, -1.0f);
                    break;
                case Command::StrafeLeft:
                    pulse(kAxisX, -1.0f);
                    break;
                case Command::StrafeRight:
                    pulse(kAxisX, 1.0f);
                    break;
                case Command::Ascend:
                    pulse(kAxisY, 1.0f);
                    break;
                case Command::Descend:
                    pulse(kAxisY, -1.0f);
                    break;
                case Command::Pause:
                    m_state = GameState::Paused;
                    break;
                case Command::Quit:
                    m_state = GameState::ShuttingDown;
                    break;
                default:
                    break;
            }
            break;

        case GameState::Paused:
            switch (cmd) {
                case Command::Pause:
                case Command::Option1:
                    m_state = GameState::Playing;
                    break;
                case Command::Option2:
                    m_state = GameState::Settings;
                    break;
                case Command::Option3:
                case Command::Quit:
                    m_state = GameState::ShuttingDown;
                    break;
                default:
                    break;
            }
            break;

        case GameState::Settings:
            if (cmd != Command::None)
                m_state = GameState::Paused;
            break;

        case GameState::GameOver:
            switch (cmd) {
                case Command::Restart:
                case Command::Option1:
                    restart();
                    break;
                case Command::Quit:
                case Command::Option3:
                    m_state = GameState::ShuttingDown;
                    break;
                default:
                    break;
            }
            break;

        case GameState::Booting:
        case GameState::ShuttingDown:
            break;
    }
}

void GameController::fixedUpdate(float dt) {
    for (int axis = 0; axis < 3; ++axis) {
        m_pulseTime[axis] -= dt;
        if (m_pulseTime[axis] <= 0.0f)
            m_pulseDir[axis] = 0.0f;
    }
    m_world.setThrustInput({m_pulseDir[kAxisX], m_pulseDir[kAxisY], m_pulseDir[kAxisZ]});

    m_world.step(dt);

    // XP por tiempo de vuelo, acumulando fracciones entre steps.
    m_xpFraction += config::kXPPerSecond * dt;
    if (m_xpFraction >= 1.0f) {
        const int whole = static_cast<int>(m_xpFraction);
        m_progression.addExperience(whole);
        m_xpFraction -= static_cast<float>(whole);
    }

    const bool exhausted = !m_world.drone().hasBattery() && m_world.drone().isGrounded() &&
                           m_world.drone().velocity().length() < 0.1f;
    if (m_crashed || exhausted) {
        m_crashed = false;
        m_state = GameState::GameOver;
    }
}

void GameController::restart() {
    m_world.reset();
    m_progression.reset();
    m_xpFraction = 0.0f;
    m_crashed = false;
    for (int axis = 0; axis < 3; ++axis) {
        m_pulseDir[axis] = 0.0f;
        m_pulseTime[axis] = 0.0f;
    }
    m_state = GameState::Playing;
}

WorldState GameController::makeState() const {
    WorldState s = m_world.snapshot();
    s.level = m_progression.getLevel();
    s.experience = m_progression.getExperience();
    s.experienceToNext = m_progression.getExperienceToNext();
    s.state = m_state;
    return s;
}

}  // namespace drone
