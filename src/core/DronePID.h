#pragma once

namespace drone {

// PID angular portado de cortex-main con la derivada sobre la MEDIDA, no
// sobre el error: así un cambio brusco de consigna no produce un pico en la
// salida (derivative kick).
//
// Roll/Pitch: PID sobre el ángulo.  Yaw: PI sobre la VELOCIDAD angular,
// sin D — la derivada de una tasa amplifica el ruido del giróscopo.
//
// UNIDADES. cortex trabaja en grados y en cuentas PWM sobre un rango de
// 1997; aquí el error va en radianes y la salida es la fracción del empuje
// total que el mezclador desvía entre motores. La conversión es un factor:
//
//     k_aquí = k_cortex · (180/π) / 1997 ≈ k_cortex · 0,0287
//
// Copiar las ganancias tal cual (Kp = 6 por radián en vez de por grado) las
// hacía ~35 veces más agresivas: saturaban con 1,4° de error en lugar de
// con los 50° del original.
struct DronePID {
    static constexpr float kFromCortex = 0.0286939f;  // (180/π)/1997

    float rollKp = 6.0f * kFromCortex;
    float rollKi = 1.5f * kFromCortex;
    float rollKd = 2.0f * kFromCortex;
    float pitchKp = 9.0f * kFromCortex;
    float pitchKi = 1.0f * kFromCortex;
    float pitchKd = 5.0f * kFromCortex;
    float yawKp = 0.15f * kFromCortex;
    float yawKi = 0.03f * kFromCortex;
    float yawFF = 1.0f * kFromCortex;

    // MAX_I_OUTPUT = 60 y MAX_PD_OUTPUT = 300, ambos sobre el rango de 1997.
    // El límite del integral debe ser MENOR que el total: antes valía 0,375
    // contra un total de 0,15, así que el integral solo ya podía saturar
    // toda la autoridad del control.
    float maxIOutput = 60.0f / 1997.0f;      // 0,030
    float maxCorrection = 300.0f / 1997.0f;  // 0,150

    void reset();

    // Correcciones normalizadas, acotadas a ±maxCorrection, para el
    // mezclador. Ángulos en radianes; tasas en rad/s.
    //
    // Las tasas son las de CADA ÁNGULO (d roll/dt, d pitch/dt, d yaw/dt), no
    // las componentes crudas de la velocidad angular del cuerpo. La relación
    // entre ambas no es la identidad —el cabeceo positivo corresponde a un
    // giro NEGATIVO sobre X— y usar la componente cruda invertía el signo de
    // la amortiguación en ese eje: en vez de frenar, realimentaba, y el dron
    // volcaba a los pocos segundos. Quien convierte es World, que conoce la
    // convención de ejes.
    //
    // targetYawRate es una TASA, no un ángulo: el yaw es un lazo de
    // velocidad, igual que en el original. Por eso el rumbo no se envuelve
    // aquí — de eso se encarga YawHold, que es quien lo conoce.
    //
    // inFlight desactiva el integral Y lo pone a cero: acumular error con el
    // dron posado produce una patada en cuanto despega.
    void compute(float targetRoll, float targetPitch, float targetYawRate, float currentRoll,
                 float currentPitch, float rollRate, float pitchRate, float yawRate, float dt,
                 bool inFlight, float& outRoll, float& outPitch, float& outYaw);

private:
    float m_iRoll = 0, m_iPitch = 0, m_iYaw = 0;
};

}  // namespace drone
