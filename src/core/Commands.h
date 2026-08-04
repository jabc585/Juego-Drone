#pragma once

namespace drone {

// Lo único que entra al core desde el frontend (PLAN2.md §6.1).
enum class Command {
    None,
    ThrustForward,
    ThrustBackward,
    StrafeLeft,
    StrafeRight,
    Ascend,
    Descend,
    Pause,
    Quit,
    Restart,
    Option1,
    Option2,
    Option3,
    Save,
    Load,
};

}  // namespace drone
