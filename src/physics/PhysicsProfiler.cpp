#include "physics/PhysicsProfiler.h"

#include <chrono>

namespace drone::physics {

double PhysicsProfiler::now() {
    using clock = std::chrono::high_resolution_clock;
    static auto t0 = clock::now();
    return std::chrono::duration<double, std::milli>(clock::now() - t0).count();
}

void PhysicsProfiler::endStep(uint32_t bodies, uint32_t awake, uint32_t events, uint32_t substeps) {
    double elapsed = now() - m_stepStart;
    m_accumMs += elapsed;
    ++m_accumFrames;

    if (m_accumFrames >= 60) {
        m_stats.msSolver = static_cast<float>(m_accumMs / m_accumFrames);
        m_accumMs = 0;
        m_accumFrames = 0;
    }

    m_stats.bodiesTotal = bodies;
    m_stats.bodiesAwake = awake;
    m_stats.eventsDispatched = events;
    m_stats.substepsThisFrame = substeps;
}

}  // namespace drone::physics
