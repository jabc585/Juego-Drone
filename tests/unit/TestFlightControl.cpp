// Tests del control de vuelo portado de cortex-main. Ninguno necesita el
// motor de física: el mezclador y el PID son funciones puras del estado, y
// eran justamente los módulos que no tenían ni un solo test cuando se
// escribieron — por eso llegaron a producción con los signos invertidos.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/AltitudeYawHold.h"
#include "core/DronePID.h"
#include "core/FailsafeTrim.h"
#include "core/XFrameMixer.h"

using namespace drone;

namespace {

// Índices dentro de XFrameMixer::Result::motors.
constexpr int kFrontLeft = 0;
constexpr int kFrontRight = 1;
constexpr int kRearLeft = 2;
constexpr int kRearRight = 3;

constexpr float kMaxThrust = 25.0f;

}  // namespace

TEST_CASE("Mixer: symmetric throttle spreads evenly and produces no torque", "[Mixer]") {
    XFrameMixer m;
    const auto r = m.compute(0.5f, 0.0f, 0.0f, 0.0f, kMaxThrust);

    for (const auto& motor : r.motors)
        REQUIRE(motor.thrust == Catch::Approx(kMaxThrust / 8.0f));
    // Los pares de reacción de las dos CW y las dos CCW se cancelan.
    REQUIRE(r.yawTorque == Catch::Approx(0.0f).margin(1e-6));
}

// La tabla de signos del plan. Con roll+ suben los motores DERECHOS, no los
// izquierdos: el criterio original decía lo contrario en las dos mitades.
TEST_CASE("Mixer: pitch+ lifts the front motors (nose up)", "[Mixer]") {
    XFrameMixer m;
    const auto r = m.compute(0.5f, 0.0f, 0.1f, 0.0f, kMaxThrust);

    REQUIRE(r.motors[kFrontLeft].thrust > r.motors[kRearLeft].thrust);
    REQUIRE(r.motors[kFrontRight].thrust > r.motors[kRearRight].thrust);
}

TEST_CASE("Mixer: roll+ lifts the right-hand motors (left side down)", "[Mixer]") {
    XFrameMixer m;
    const auto r = m.compute(0.5f, 0.1f, 0.0f, 0.0f, kMaxThrust);

    REQUIRE(r.motors[kFrontRight].thrust > r.motors[kFrontLeft].thrust);
    REQUIRE(r.motors[kRearRight].thrust > r.motors[kRearLeft].thrust);
}

TEST_CASE("Mixer: yaw+ lifts the counter-clockwise motors", "[Mixer]") {
    XFrameMixer m;
    const auto r = m.compute(0.5f, 0.0f, 0.0f, 0.1f, kMaxThrust);

    // Las CCW son la delantera izquierda y la trasera derecha.
    REQUIRE(r.motors[kFrontLeft].thrust > r.motors[kFrontRight].thrust);
    REQUIRE(r.motors[kRearRight].thrust > r.motors[kRearLeft].thrust);
    // Y ahora sí hay par sobre el eje vertical.
    REQUIRE(r.yawTorque > 0.0f);
}

TEST_CASE("Mixer: without reaction torque yaw does nothing at all", "[Mixer]") {
    XFrameMixer m;
    m.yawTorqueFactor = 0.0f;
    const auto r = m.compute(0.5f, 0.0f, 0.0f, 0.5f, kMaxThrust);

    // Las cuatro fuerzas son paralelas al eje vertical del cuerpo: sin el
    // par de reacción ninguna genera giro. Era el estado del código antes.
    REQUIRE(r.yawTorque == Catch::Approx(0.0f).margin(1e-6));
}

TEST_CASE("Mixer: motor positions follow the arm length at 45 degrees", "[Mixer]") {
    XFrameMixer m;
    m.armLength = 0.25f;
    const auto r = m.compute(0.5f, 0.0f, 0.0f, 0.0f, kMaxThrust);

    const float d = 0.25f * 0.70710678f;
    REQUIRE(std::fabs(r.motors[kFrontLeft].position.x) == Catch::Approx(d));
    REQUIRE(std::fabs(r.motors[kFrontLeft].position.z) == Catch::Approx(d));
    // Cada motor está a armLength del centro, no a armLength·√2.
    const auto& p = r.motors[kFrontRight].position;
    REQUIRE(std::sqrt(p.x * p.x + p.z * p.z) == Catch::Approx(0.25f));
}

TEST_CASE("Mixer: a saturated motor never pushes backwards", "[Mixer]") {
    XFrameMixer m;
    const auto r = m.compute(1.0f, 1.0f, 1.0f, 1.0f, kMaxThrust);
    for (const auto& motor : r.motors) {
        REQUIRE(motor.thrust >= 0.0f);
        REQUIRE(motor.thrust <= kMaxThrust / 4.0f + 1e-4f);
    }
}

TEST_CASE("PID: an attitude error is corrected towards the setpoint", "[PID]") {
    DronePID pid;
    float roll = 0, pitch = 0, yaw = 0;
    // Morro 10° abajo, se pide nivelado: la corrección debe ser positiva.
    pid.compute(0.0f, 0.0f, 0.0f, 0.0f, -0.175f, 0, 0, 0, 1.0f / 60.0f, true, roll, pitch, yaw);
    REQUIRE(pitch > 0.0f);
}

TEST_CASE("PID: the derivative damps, it does not amplify", "[PID]") {
    DronePID pid;
    float roll = 0, pitch = 0, yaw = 0;

    // Sin error de ángulo pero cabeceando hacia arriba: el término derivativo
    // debe oponerse. Con el signo invertido realimentaba, y el dron volcaba a
    // los pocos segundos de vuelo.
    pid.compute(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f / 60.0f, true, roll, pitch,
                yaw);
    REQUIRE(pitch < 0.0f);

    pid.reset();
    pid.compute(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f / 60.0f, true, roll, pitch,
                yaw);
    REQUIRE(roll < 0.0f);
}

TEST_CASE("PID: the integral does not wind up while grounded", "[PID]") {
    DronePID pid;
    float roll = 0, pitch = 0, yaw = 0;

    // 10 s de error sostenido con el dron en el suelo.
    for (int i = 0; i < 600; ++i)
        pid.compute(0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 1.0f / 60.0f, false, roll, pitch, yaw);
    REQUIRE(roll == 0.0f);
    REQUIRE(pitch == 0.0f);

    // Al despegar, la primera salida es solo proporcional: si el integral
    // hubiese seguido acumulando, aquí llegaría una patada.
    pid.compute(0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 1.0f / 60.0f, true, roll, pitch, yaw);
    REQUIRE(roll == Catch::Approx(pid.rollKp * 0.5f).epsilon(0.01));
}

TEST_CASE("PID: the output never exceeds the configured authority", "[PID]") {
    DronePID pid;
    float roll = 0, pitch = 0, yaw = 0;
    for (int i = 0; i < 600; ++i)
        pid.compute(3.0f, 3.0f, 50.0f, 0.0f, 0.0f, 0, 0, 0, 1.0f / 60.0f, true, roll, pitch, yaw);
    REQUIRE(std::fabs(roll) <= pid.maxCorrection + 1e-6f);
    REQUIRE(std::fabs(pitch) <= pid.maxCorrection + 1e-6f);
    REQUIRE(std::fabs(yaw) <= pid.maxCorrection + 1e-6f);
}

TEST_CASE("PID: gains carry the unit conversion from the original", "[PID]") {
    const DronePID pid;
    // 6.0 en las unidades de cortex (grados y cuentas PWM) son ~0,172 aquí.
    // Copiarlas crudas las hacía ~35 veces más agresivas.
    REQUIRE(pid.rollKp == Catch::Approx(6.0f * 0.0286939f));
    // Y el tope del integral debe ser menor que el total, no mayor.
    REQUIRE(pid.maxIOutput < pid.maxCorrection);
}

TEST_CASE("AltitudeHold: holds its target and saturates the correction", "[Hold]") {
    AltitudeHold hold;
    REQUIRE(hold.compute(10.0f) == 0.0f);  // desactivado, no corrige

    hold.toggle(10.0f);
    REQUIRE(hold.engaged);
    REQUIRE(hold.targetAltitude == 10.0f);
    REQUIRE(hold.compute(10.0f) == Catch::Approx(0.0f));
    REQUIRE(hold.compute(8.0f) > 0.0f);   // por debajo ⇒ más gas
    REQUIRE(hold.compute(12.0f) < 0.0f);  // por encima ⇒ menos gas
    REQUIRE(hold.compute(-1000.0f) <= hold.maxCorrection);

    hold.toggle(10.0f);
    REQUIRE_FALSE(hold.engaged);
}

TEST_CASE("YawHold: locks the heading when the stick is released", "[Hold]") {
    YawHold hold;
    // Mando fuera de la zona muerta: control de tasa directo, sin bloqueo.
    REQUIRE(hold.compute(0.8f, 1.0f) == Catch::Approx(0.8f));
    REQUIRE_FALSE(hold.targetSet);

    // Al soltar, engancha el rumbo actual y no pide giro.
    REQUIRE(hold.compute(0.0f, 1.0f) == Catch::Approx(0.0f));
    REQUIRE(hold.targetSet);
    REQUIRE(hold.targetHeading == Catch::Approx(1.0f));
    // Si el rumbo se desvía, pide una tasa que lo devuelve.
    REQUIRE(hold.compute(0.0f, 0.9f) > 0.0f);
}

TEST_CASE("YawHold: heading error wraps the short way round", "[Hold]") {
    YawHold hold;
    const float pi = 3.14159265f;
    hold.compute(0.0f, pi - 0.05f);  // engancha cerca de +π

    // El rumbo cruza a −π: la corrección debe ser pequeña y en el sentido
    // corto, no un giro de casi 360°.
    const float rate = hold.compute(0.0f, -pi + 0.05f);
    REQUIRE(std::fabs(rate) < hold.holdKp * 0.5f);
}

TEST_CASE("Failsafe: does not engage while grounded", "[Failsafe]") {
    Failsafe fs;
    for (int i = 0; i < 600; ++i)
        fs.compute(0.0f, 0.5f, /*grounded=*/true, 1.0f / 60.0f);
    REQUIRE_FALSE(fs.active);
}

TEST_CASE("Failsafe: airborne with no input it descends from hover", "[Failsafe]") {
    Failsafe fs;
    fs.timeoutSeconds = 1.0f;

    float out = 1.0f;
    for (int i = 0; i < 60; ++i)  // 1 s: aún dentro del timeout
        out = fs.compute(0.0f, 0.5f, false, 1.0f / 60.0f);
    REQUIRE_FALSE(fs.active);

    out = fs.compute(0.0f, 0.5f, false, 1.0f / 60.0f);
    REQUIRE(fs.active);
    // Arranca en el empuje de hover, no en cero: antes empezaba en cero y no
    // había descenso ninguno que controlar.
    REQUIRE(out > 0.4f);

    for (int i = 0; i < 600; ++i)
        out = fs.compute(0.0f, 0.5f, false, 1.0f / 60.0f);
    REQUIRE(out == Catch::Approx(0.0f));
}

TEST_CASE("Failsafe: any input cancels it immediately", "[Failsafe]") {
    Failsafe fs;
    fs.timeoutSeconds = 0.5f;
    for (int i = 0; i < 120; ++i)
        fs.compute(0.0f, 0.5f, false, 1.0f / 60.0f);
    REQUIRE(fs.active);

    const float out = fs.compute(0.8f, 0.5f, false, 1.0f / 60.0f);
    REQUIRE_FALSE(fs.active);
    REQUIRE(out == Catch::Approx(0.8f));
}

TEST_CASE("AttitudeTrim: accumulates and is bounded", "[Trim]") {
    AttitudeTrim trim;
    trim.addPitch(0.2f);
    trim.addPitch(0.2f);
    REQUIRE(trim.pitchDeg == Catch::Approx(0.4f));

    // Un trim sin tope acaba en una consigna que el dron no puede volar.
    for (int i = 0; i < 1000; ++i)
        trim.addRoll(0.2f);
    REQUIRE(trim.rollDeg == Catch::Approx(AttitudeTrim::kLimitDeg));

    trim.reset();
    REQUIRE(trim.pitchDeg == 0.0f);
    REQUIRE(trim.rollDeg == 0.0f);
}
