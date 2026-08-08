#pragma once

namespace drone {

enum class CameraMode { Follow, Free, Orbit };

// Estado de presentación que comparten la entrada y el renderizador.
//
// Hace falta porque la cámara libre de raylib (CAMERA_FIRST_PERSON) lee W/A/S/D
// por su cuenta para moverse. Si la entrada sigue mandando esas mismas teclas
// al dron, mover la cámara pilota el dron a la vez. Ninguno de los dos puede
// decidirlo solo: el renderizador es quien sabe en qué cámara estamos y la
// entrada es quien tiene que callarse.
struct RaylibViewState {
    CameraMode cameraMode = CameraMode::Follow;

    // Con la cámara libre, el teclado de movimiento es suyo.
    bool freeCameraOwnsMovement() const { return cameraMode == CameraMode::Free; }
};

}  // namespace drone
