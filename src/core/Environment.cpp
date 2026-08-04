#include "core/Environment.h"

#include <algorithm>
#include <cmath>

namespace drone {

Environment::Environment(const GameConfig& cfg) : m_config(cfg), m_rng(0) {
    scheduleNextGust();
}

void Environment::loadEnvironment(const std::string& environmentName) {
    m_name = environmentName;
    m_obstacles = {
        {{8.0f, 5.0f, 8.0f}, {2.0f, 10.0f, 2.0f}},
        {{-10.0f, 4.0f, 6.0f}, {3.0f, 8.0f, 3.0f}},
        {{5.0f, 6.0f, -12.0f}, {2.5f, 12.0f, 2.5f}},
    };
}

void Environment::setSeed(uint32_t seed) {
    m_seed = seed;
    m_rng.seed(seed);
    scheduleNextGust();
}

void Environment::scheduleNextGust() {
    std::uniform_real_distribution<float> interval(m_config.gustMinInterval,
                                                   m_config.gustMaxInterval);
    m_timeToNextGust = interval(m_rng);
}

void Environment::step(float dt) {
    m_elapsed += dt;
    m_difficulty = std::min(m_config.maxDifficulty, 1.0f + m_elapsed * m_config.difficultyRamp);

    m_timeToNextGust -= dt;
    if (m_timeToNextGust <= 0.0f) {
        std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
        std::uniform_real_distribution<float> magDist(0.5f, 1.5f);
        std::uniform_real_distribution<float> vertDist(-0.1f, 0.1f);
        const float angle = angleDist(m_rng);
        const float magnitude = magDist(m_rng) * m_difficulty * m_config.windBaseSpeed;
        m_windTarget = {std::cos(angle) * magnitude, vertDist(m_rng) * magnitude,
                        std::sin(angle) * magnitude};
        scheduleNextGust();
    }

    const float blend = std::min(1.0f, m_config.windSmoothing * dt);
    m_wind += (m_windTarget - m_wind) * blend;
}

void Environment::restoreProgress(float elapsed) {
    m_elapsed = std::max(0.0f, elapsed);
    m_difficulty = std::min(m_config.maxDifficulty, 1.0f + m_elapsed * m_config.difficultyRamp);
}

void Environment::reset() {
    m_wind = {};
    m_windTarget = {};
    m_difficulty = 1.0f;
    m_elapsed = 0.0f;
    m_rng.seed(m_seed);
    scheduleNextGust();
}

}  // namespace drone
