#pragma once

#include <cstdint>

namespace drone::physics {

// Metricas de rendimiento del motor de fisica (grafico.md §6.18).
// Entra en la Fase 1 para que ninguna optimizacion se haga sin datos.
struct PhysicsStats {
    uint32_t bodiesTotal = 0;
    uint32_t bodiesAwake = 0;
    uint32_t contactPairs = 0;
    uint32_t eventsDispatched = 0;
    uint32_t substepsThisFrame = 0;
    float msSolver = 0;  // media movil del tiempo en world->update()
};

class PhysicsProfiler {
public:
    void beginStep() { m_stepStart = now(); }
    void endStep(uint32_t bodies, uint32_t awake, uint32_t events, uint32_t substeps);

    const PhysicsStats& stats() const { return m_stats; }

private:
    static double now();
    PhysicsStats m_stats;
    double m_stepStart = 0;
    double m_accumMs = 0;
    int m_accumFrames = 0;
};

}  // namespace drone::physics
