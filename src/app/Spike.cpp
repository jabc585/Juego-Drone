#include <raylib.h>
#include <rlgl.h>

#include <cstdio>

#include "physics/PhysicsManager.h"
#include "physics/PhysicsSettings.h"

using namespace drone::physics;

int main() {
    InitWindow(800, 600, "rp3d Spike");
    SetTargetFPS(60);

    Camera3D camera = {};
    camera.position = {0, 3, 10};
    camera.target = {0, 2, 0};
    camera.up = {0, 1, 0};
    camera.fovy = 60;
    camera.projection = CAMERA_PERSPECTIVE;

    PhysicsSettings settings;
    PhysicsManager pm(settings);

    pm.createBoxBody({5.0f, 0.5f, 5.0f}, {0, -0.5f, 0}, BodyType::Static);
    auto box = pm.createBoxBody({0.5f, 0.5f, 0.5f}, {0, 5.0f, 0}, BodyType::Dynamic, 1.0f);

    while (!WindowShouldClose()) {
        pm.step(1.0f / 60.0f);
        auto t = pm.getTransform(box);

        BeginDrawing();
        ClearBackground({30, 30, 50, 255});
        BeginMode3D(camera);
        DrawCube({0, -0.5f, 0}, 10.0f, 1.0f, 10.0f, DARKGREEN);
        DrawCube({t.position.x, t.position.y, t.position.z}, 1.0f, 1.0f, 1.0f, RED);
        DrawGrid(10, 1.0f);
        EndMode3D();
        DrawFPS(10, 10);
        char buf[64];
        snprintf(buf, sizeof(buf), "Pos: (%.2f, %.2f, %.2f)", t.position.x, t.position.y,
                 t.position.z);
        DrawText(buf, 10, 40, 20, WHITE);
        EndDrawing();
    }
    return 0;
}
