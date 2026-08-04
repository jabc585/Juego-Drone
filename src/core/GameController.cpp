#include "core/GameController.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace drone {

namespace {
constexpr int kAxisX = 0;
constexpr int kAxisY = 1;
constexpr int kAxisZ = 2;
constexpr int kMaxCommandsPerFrame = 32;
}  // namespace

GameController::GameController(IInputSource& input, IRenderer& renderer, const GameConfig& cfg)
    : m_config(cfg),
      m_input(input),
      m_renderer(renderer),
      m_world(cfg),
      m_progression(cfg, m_world.events()) {
    m_world.environment().loadEnvironment("Ciudad Futurista");

    m_world.events().subscribe(EventType::Collision, [this](const Event& e) {
        if (e.value > m_config.crashSpeed)
            m_crashed = true;
    });
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
    m_accumulator += std::min(frameSeconds, m_config.maxFrameTime);

    for (int i = 0; i < kMaxCommandsPerFrame; ++i) {
        const Command cmd = m_input.poll();
        if (cmd == Command::None)
            break;
        handleCommand(cmd);
        if (m_state == GameState::ShuttingDown)
            return;
    }

    while (m_accumulator >= m_config.fixedTimestep) {
        if (m_state == GameState::Playing)
            fixedUpdate(m_config.fixedTimestep);
        m_accumulator -= m_config.fixedTimestep;
    }

    m_renderer.draw(makeState(), m_accumulator / m_config.fixedTimestep);
}

void GameController::handleCommand(Command cmd) {
    const auto pulse = [this](int axis, float dir) {
        m_pulseDir[axis] = dir;
        m_pulseTime[axis] = m_config.thrustPulseSeconds;
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
                case Command::Save:
                    m_saveRequested = true;
                    break;
                case Command::Load:
                    m_loadRequested = true;
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

    m_xpFraction += m_config.xpPerSecond * dt;
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

void GameController::applyLoad(const SaveData& data) {
    m_world.reset();
    m_progression.reset();
    m_xpFraction = 0.0f;
    m_crashed = false;
    for (int axis = 0; axis < 3; ++axis) {
        m_pulseDir[axis] = 0.0f;
        m_pulseTime[axis] = 0.0f;
    }

    auto& drone = m_world.drone();
    drone.setPosition({data.dronePosX, data.dronePosY, data.dronePosZ});
    drone.setVelocity({data.droneVelX, data.droneVelY, data.droneVelZ});
    drone.drainBattery(drone.battery() - data.battery);

    // Sin eventos: cargar nivel 5 no debe disparar cuatro LevelUp.
    m_progression.restore(data.level, data.experience);
    m_world.restoreSimTime(data.simTime);

    // Un save solo puede reanudarse jugando o en pausa; cualquier otro
    // estado guardado (corrupto o editado) no debe cerrar el juego.
    m_state = (data.state == GameState::Paused) ? GameState::Paused : GameState::Playing;
}

}  // namespace drone
