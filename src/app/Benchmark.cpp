#include <chrono>
#include <cstdio>
#include <vector>

#include "physics/PhysicsManager.h"
#include "physics/PhysicsSettings.h"

using namespace drone::physics;

int main() {
    PhysicsSettings settings;
    PhysicsManager pm(settings);

    pm.createBoxBody({50.0f, 0.5f, 50.0f}, {0, -0.5f, 0}, BodyType::Static);

    const int N = 1000;
    std::vector<BodyId> boxes(N);
    for (int i = 0; i < N; ++i) {
        float x = (i % 10) * 1.2f - 5.5f;
        float y = (i / 100) * 1.1f + 0.5f;
        float z = ((i / 10) % 10) * 1.2f - 5.5f;
        boxes[i] = pm.createBoxBody({0.5f, 0.5f, 0.5f}, {x, y, z}, BodyType::Dynamic, 1.0f);
    }

    for (int i = 0; i < 120; ++i)
        pm.step(1.0f / 60.0f);

    const int steps = 600;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; ++i)
        pm.step(1.0f / 60.0f);
    auto t1 = std::chrono::high_resolution_clock::now();

    auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    double msPerStep = totalUs / 1000.0 / steps;
    double fps = 1000.0 / msPerStep;

    std::printf("=== rp3d Benchmark (Fase 0) ===\n");
    std::printf("Cuerpos: %d cajas dinamicas + 1 suelo estatico\n", N);
    std::printf("Pasos medidos: %d a 1/60 s\n", steps);
    std::printf("Tiempo total: %.2f ms\n", totalUs / 1000.0);
    std::printf("ms/paso: %.3f\n", msPerStep);
    std::printf("FPS equivalente: %.0f\n", fps);
    std::printf("Presupuesto a 60 FPS: 16.67 ms -> %.1f%% ocupado\n", msPerStep / 16.67 * 100.0);
    return 0;
}
