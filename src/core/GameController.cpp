#include "core/GameController.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace drone {

namespace {
constexpr int kAxisX = 0, kAxisZ = 2;
constexpr int kMaxCommandsPerFrame = 32;

// Barra espaciadora. Dos pulsaciones separadas por menos de esto son "sube
// sin parar"; una suelta es un salto de un metro. La repeticion automatica
// del teclado cae dentro de la ventana, asi que mantener la barra tambien
// deja el ascenso fijo en vez de encadenar saltos.
constexpr float kDoubleTapSeconds = 0.35f;
constexpr float kElevateStepMeters = 1.0f;

// Un salto de un metro dura medio segundo. Si en este plazo no ha llegado es
// que algo lo impide (volcado, contra un techo, sin bateria); sin este tope
// el mando se quedaba pidiendo empuje pleno para siempre.
constexpr float kStepTimeoutSeconds = 2.0f;
}  // namespace

GameController::GameController(IInputSource& input, IRenderer& renderer, const GameConfig& cfg,
                               const physics::PhysicsSettings& physCfg)
    : m_config(cfg),
      m_input(input),
      m_renderer(renderer),
      m_world(cfg, physCfg),
      m_progression(cfg, m_world.events()) {
    m_gravity = std::fabs(physCfg.gravity.y);
    m_world.loadEnvironment("Ciudad Futurista");

    m_world.events().subscribe(EventType::Collision, [this](const Event& e) {
        if (e.value > m_config.crashSpeed)
            m_crashed = true;
    });
    m_world.events().subscribe(EventType::LandingZone, [this](const Event& e) {
        m_progression.addExperience(static_cast<int>(e.value));
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
    m_inputClock += frameSeconds;
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
                    onElevate();
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
                case Command::AltitudeToggle:
                    m_world.altitudeHold().toggle(m_world.drone().position().y);
                    m_climb = ClimbMode::Off;
                    break;
                case Command::TrimPitchUp:
                    m_world.trim().addPitch(m_config.trimStepDeg);
                    break;
                case Command::TrimPitchDown:
                    m_world.trim().addPitch(-m_config.trimStepDeg);
                    break;
                case Command::TrimRollLeft:
                    m_world.trim().addRoll(-m_config.trimStepDeg);
                    break;
                case Command::TrimRollRight:
                    m_world.trim().addRoll(m_config.trimStepDeg);
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
        // Enumerados explícitamente y no con default: así, al añadir un
        // estado nuevo, -Wswitch avisa de que falta tratarlo aquí.
        case GameState::Booting:
        case GameState::ShuttingDown:
            break;
    }
}

// La barra no da empuje directo: cuenta pulsaciones. Una sola pide un metro
// mas de altura; dos seguidas dejan el ascenso fijo, y la siguiente pulsacion
// suelta lo corta. No hay bajada porque de eso ya se encarga la gravedad.
void GameController::onElevate() {
    const float sinceLast = m_inputClock - m_lastElevate;
    m_lastElevate = m_inputClock;

    if (sinceLast <= kDoubleTapSeconds) {
        m_climb = ClimbMode::Continuous;
    } else if (m_climb == ClimbMode::Continuous) {
        m_climb = ClimbMode::Off;
    } else {
        m_climb = ClimbMode::Step;
        m_climbTargetY = m_world.drone().position().y + kElevateStepMeters;
        m_climbUntil = m_inputClock + kStepTimeoutSeconds;
    }
}

// A empuje pleno el dron sigue subiendo por inercia mucho despues de soltar,
// asi que para clavar el metro hay que cortar antes: se compara lo que falta
// con lo que aun subiria en caida libre con la velocidad que ya lleva.
float GameController::climbThrottle() {
    switch (m_climb) {
        case ClimbMode::Off:
            return 0.0f;
        case ClimbMode::Continuous:
            return 1.0f;
        case ClimbMode::Step:
            break;
    }

    if (m_inputClock > m_climbUntil) {
        m_climb = ClimbMode::Off;  // no ha podido: no se insiste
        return 0.0f;
    }

    const float y = m_world.drone().position().y;
    const float vy = m_world.drone().velocity().y;
    const float remaining = m_climbTargetY - y;
    if (remaining <= 0.0f) {
        m_climb = ClimbMode::Off;
        return 0.0f;
    }
    // Frena la gravedad y ademas el rozamiento del aire, que a estas
    // velocidades no es despreciable: contar solo con g predecia una inercia
    // mayor de la real, se cortaba antes de tiempo y el salto se quedaba en
    // 0,8 m de los 1,0 pedidos.
    // El rozamiento se evalua a la velocidad MEDIA del tramo (vy/2): con la
    // instantanea se sobreestimaba el frenado y el salto se pasaba a 1,25 m.
    const float decel = m_gravity + (m_config.droneMass > 0.0f ? m_config.dragCoefficient * vy /
                                                                     (2.0f * m_config.droneMass)
                                                               : 0.0f);
    const float coast = (vy > 0.0f && decel > 0.0f) ? (vy * vy) / (2.0f * decel) : 0.0f;

    // Los motores no se paran en seco: durante su constante de tiempo el dron
    // sigue empujando y gana este tramo de mas. Sin descontarlo el salto se
    // pasaba a 1,15 m.
    const float lagRise = std::max(0.0f, vy) * m_config.motorTimeConstant;

    if (coast + lagRise >= remaining) {
        m_climb = ClimbMode::Off;  // con la inercia que lleva ya llega
        return 0.0f;
    }
    return 1.0f;
}

void GameController::fixedUpdate(float dt) {
    for (const int axis : {kAxisX, kAxisZ}) {
        m_pulseTime[axis] -= dt;
        if (m_pulseTime[axis] <= 0.0f)
            m_pulseDir[axis] = 0.0f;
    }
    m_world.setThrustInput({m_pulseDir[kAxisX], climbThrottle(), m_pulseDir[kAxisZ]});
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
    m_climb = ClimbMode::Off;
    m_lastElevate = -1000.0f;
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
    m_climb = ClimbMode::Off;
    m_lastElevate = -1000.0f;

    // Por teleportDrone y no por Drone::setPosition: hay que mover el cuerpo
    // de rp3d, que es la fuente de verdad. Escribir solo el estado de juego
    // no tendría efecto — el siguiente paso lo sobrescribe.
    m_world.teleportDrone({data.dronePosX, data.dronePosY, data.dronePosZ},
                          {data.droneVelX, data.droneVelY, data.droneVelZ});
    m_world.setDroneOrientation(data.droneQx, data.droneQy, data.droneQz, data.droneQw);
    m_world.drone().drainBattery(m_world.drone().battery() - data.battery);

    // Sin eventos: cargar nivel 5 no debe disparar cuatro LevelUp.
    m_progression.restore(data.level, data.experience);
    m_world.restoreSimTime(data.simTime);

    // Un save solo puede reanudarse jugando o en pausa; cualquier otro
    // estado guardado (corrupto o editado) no debe cerrar el juego.
    m_state = (data.state == GameState::Paused) ? GameState::Paused : GameState::Playing;
}

}  // namespace drone
