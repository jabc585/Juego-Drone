#include "core/AltitudeYawHold.h"

#include <algorithm>
#include <cmath>

namespace drone {

namespace {

// M_PI es POSIX, no C++ estándar: en MSVC no existe sin _USE_MATH_DEFINES y
// la matriz de CI compila también en Windows.
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

// Lleva un error angular a [-π, π] de una vez, sin bucle: un rumbo corrupto
// (NaN o enorme) colgaría un while.
float wrapPi(float angle) {
    angle = std::fmod(angle + kPi, kTwoPi);
    if (angle < 0.0f)
        angle += kTwoPi;
    return angle - kPi;
}

}  // namespace

void AltitudeHold::reset() {
    engaged = false;
    targetAltitude = 0;
}

void AltitudeHold::toggle(float currentAltitude) {
    if (engaged) {
        reset();
        return;
    }
    engaged = true;
    targetAltitude = currentAltitude;
}

float AltitudeHold::compute(float currentAltitude) const {
    if (!engaged)
        return 0.0f;
    const float correction = kp * (targetAltitude - currentAltitude);
    return std::max(-maxCorrection, std::min(maxCorrection, correction));
}

void YawHold::reset() {
    targetSet = false;
    targetHeading = 0;
}

float YawHold::compute(float stickInput, float currentHeading) {
    if (std::fabs(stickInput) > deadzone) {
        targetSet = false;
        return stickInput;  // control de tasa normal
    }
    if (!targetSet) {
        targetHeading = currentHeading;
        targetSet = true;
    }
    return holdKp * wrapPi(targetHeading - currentHeading);
}

}  // namespace drone
