#pragma once

namespace drone {

// Lo único que entra al core desde el frontend (PLAN2.md §6.1).
enum class Command {
    None,
    // Se pulso una tecla sin accion asignada. Distinguirla de None permite que
    // pantallas como Configuracion vuelvan con cualquier tecla, como anuncian.
    Unknown,
    ThrustForward,
    ThrustBackward,
    StrafeLeft,
    StrafeRight,
    // Barra espaciadora. Una pulsacion sube un metro; dos seguidas dejan el
    // ascenso fijo. No hay mando de bajada: de eso se encarga la gravedad.
    Ascend,
    Pause,
    Quit,
    Restart,
    Option1,
    Option2,
    Option3,
    Save,
    Load,
    AltitudeToggle,
    TrimPitchUp,
    TrimPitchDown,
    TrimRollLeft,
    TrimRollRight,
};

}  // namespace drone
