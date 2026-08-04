#include "frontend/raylib/RaylibInput.h"

#include <raylib.h>

namespace drone {

Command RaylibInput::poll() {
    if (WindowShouldClose())
        return Command::Quit;

    if (IsKeyPressed(KEY_W))
        return Command::ThrustForward;
    if (IsKeyPressed(KEY_S))
        return Command::ThrustBackward;
    if (IsKeyPressed(KEY_A))
        return Command::StrafeLeft;
    if (IsKeyPressed(KEY_D))
        return Command::StrafeRight;
    if (IsKeyPressed(KEY_Q))
        return Command::Ascend;
    if (IsKeyPressed(KEY_E))
        return Command::Descend;
    if (IsKeyPressed(KEY_UP))
        return Command::ThrustForward;
    if (IsKeyPressed(KEY_DOWN))
        return Command::ThrustBackward;
    if (IsKeyPressed(KEY_LEFT))
        return Command::StrafeLeft;
    if (IsKeyPressed(KEY_RIGHT))
        return Command::StrafeRight;
    if (IsKeyPressed(KEY_P))
        return Command::Pause;
    if (IsKeyPressed(KEY_ESCAPE))
        return Command::Quit;
    if (IsKeyPressed(KEY_F5))
        return Command::Save;
    if (IsKeyPressed(KEY_F9))
        return Command::Load;
    if (IsKeyPressed(KEY_R))
        return Command::Restart;
    if (IsKeyPressed(KEY_ONE))
        return Command::Option1;
    if (IsKeyPressed(KEY_TWO))
        return Command::Option2;
    if (IsKeyPressed(KEY_THREE))
        return Command::Option3;

    return Command::None;
}

}  // namespace drone
