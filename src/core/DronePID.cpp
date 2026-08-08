#include "core/DronePID.h"

#include <algorithm>

namespace drone {

namespace {

float clamped(float v, float limit) {
    return std::max(-limit, std::min(limit, v));
}

}  // namespace

void DronePID::reset() {
    m_iRoll = m_iPitch = m_iYaw = 0.0f;
}

void DronePID::compute(float targetRoll, float targetPitch, float targetYawRate, float currentRoll,
                       float currentPitch, float rollRate, float pitchRate, float yawRate, float dt,
                       bool inFlight, float& outRoll, float& outPitch, float& outYaw) {
    if (!inFlight) {
        // Poner a cero, no solo ignorar: si el error se sigue acumulando con
        // el dron posado, al despegar se libera de golpe.
        reset();
        outRoll = outPitch = outYaw = 0.0f;
        return;
    }

    // Derivada sobre la MEDIDA: se resta la tasa del propio ángulo, así que
    // el término siempre frena el movimiento en curso.
    const float errRoll = targetRoll - currentRoll;
    m_iRoll = clamped(m_iRoll + rollKi * errRoll * dt, maxIOutput);
    outRoll = clamped(rollKp * errRoll + m_iRoll - rollKd * rollRate, maxCorrection);

    const float errPitch = targetPitch - currentPitch;
    m_iPitch = clamped(m_iPitch + pitchKi * errPitch * dt, maxIOutput);
    outPitch = clamped(pitchKp * errPitch + m_iPitch - pitchKd * pitchRate, maxCorrection);

    // Yaw: lazo de velocidad. El error va entre la tasa pedida y la medida,
    // y el feedforward sobre la tasa pedida.
    const float errYaw = targetYawRate - yawRate;
    m_iYaw = clamped(m_iYaw + yawKi * errYaw * dt, maxIOutput);
    outYaw = clamped(yawKp * errYaw + m_iYaw + yawFF * targetYawRate, maxCorrection);
}

}  // namespace drone
