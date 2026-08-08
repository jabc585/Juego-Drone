#include <catch2/catch_test_macros.hpp>

#include "frontend/raylib/ParticleSystem.h"

using drone::particleAlpha;
using drone::ParticleSystem;

// El fallo que motivó estos tests: el polvo de despegue se emite con vida 1,5
// y `life * 255` no cabe en un unsigned char. Convertirlo sin acotar es
// comportamiento indefinido —UBSan lo cazaba en el primer frame dibujado— y
// no había ninguna prueba que lo cubriera porque el cálculo vivía dentro de
// draw(), que necesita un contexto de OpenGL.
TEST_CASE("particleAlpha acota la vida al rango de un byte", "[Particles]") {
    REQUIRE(particleAlpha(0.0f) == 0);
    REQUIRE(particleAlpha(1.0f) == 255);
    REQUIRE(particleAlpha(0.5f) == 127);

    // Por encima de 1: es lo que emite el despegue.
    REQUIRE(particleAlpha(1.5f) == 255);
    REQUIRE(particleAlpha(1000.0f) == 255);

    // Y por debajo de 0, que es donde queda una partícula ya agotada.
    REQUIRE(particleAlpha(-0.1f) == 0);
    REQUIRE(particleAlpha(-1000.0f) == 0);
}

TEST_CASE("ParticleSystem: emitir añade justo las partículas pedidas", "[Particles]") {
    ParticleSystem ps;
    REQUIRE(ps.count() == 0);

    ps.emit({0, 0, 0}, 20, 5.0f, 8.0f, 0.5f, 0.02f, {255, 200, 50, 255});
    REQUIRE(ps.count() == 20);

    ps.emitRing({0, 0, 0}, 15, 0.5f, 2.0f, 1.5f, 0.04f, {180, 150, 100, 255});
    REQUIRE(ps.count() == 35);
}

TEST_CASE("ParticleSystem: las partículas caducan y se retiran", "[Particles]") {
    // La vida de cada partícula es la pedida por un factor aleatorio de
    // [0,5 · 1,0), así que las cotas se razonan sobre el rango, no sobre la
    // secuencia concreta de rand(): con vida 1,0 ninguna pasa de 1 s.
    ParticleSystem ps;
    ps.emitRing({0, 0, 0}, 30, 0.5f, 2.0f, 1.0f, 0.04f, {180, 150, 100, 255});
    REQUIRE(ps.count() == 30);

    for (int i = 0; i < 60; ++i)
        ps.update(1.0f / 60.0f);
    REQUIRE(ps.count() == 0);
}

// Rangos que no se solapan: con vida 0,2 ninguna llega a 0,2 s y con vida 2,0
// todas pasan de 1 s. Así la comprobación no depende del azar.
TEST_CASE("ParticleSystem: una vida mayor dura más", "[Particles]") {
    ParticleSystem cortas, largas;
    cortas.emit({0, 0, 0}, 20, 1.0f, 1.0f, 0.2f, 0.1f, {255, 255, 255, 255});
    largas.emit({0, 0, 0}, 20, 1.0f, 1.0f, 2.0f, 0.1f, {255, 255, 255, 255});

    for (int i = 0; i < 30; ++i) {  // medio segundo
        cortas.update(1.0f / 60.0f);
        largas.update(1.0f / 60.0f);
    }
    REQUIRE(cortas.count() == 0);
    REQUIRE(largas.count() == 20);
}

TEST_CASE("ParticleSystem: un paso de cero no las mata", "[Particles]") {
    ParticleSystem ps;
    ps.emit({0, 0, 0}, 10, 1.0f, 1.0f, 1.0f, 0.1f, {255, 255, 255, 255});
    ps.update(0.0f);
    REQUIRE(ps.count() == 10);
}
