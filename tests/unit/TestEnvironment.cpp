#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <vector>

#include "core/Environment.h"
#include "core/GameConfig.h"
#include "core/math/Vec3.h"

using drone::Environment;
using drone::GameConfig;
using drone::Vec3;

TEST_CASE("Environment starts with wind zero and difficulty 1", "[Environment]") {
    GameConfig cfg;
    Environment env(cfg);
    REQUIRE(env.wind().length() == 0.0f);
    REQUIRE(env.difficulty() == 1.0f);
}

TEST_CASE("Environment same seed produces same gusts", "[Environment]") {
    GameConfig cfg;
    cfg.gustMinInterval = 0.1f;
    cfg.gustMaxInterval = 0.1f;

    Environment a(cfg);
    Environment b(cfg);
    a.setSeed(42);
    b.setSeed(42);

    for (int i = 0; i < 600; ++i) {
        a.step(cfg.fixedTimestep);
        b.step(cfg.fixedTimestep);
    }

    REQUIRE(a.wind().x == b.wind().x);
    REQUIRE(a.wind().y == b.wind().y);
    REQUIRE(a.wind().z == b.wind().z);
}

TEST_CASE("Environment different seeds produce different gusts", "[Environment]") {
    GameConfig cfg;
    cfg.gustMinInterval = 0.1f;
    cfg.gustMaxInterval = 0.1f;

    Environment a(cfg);
    Environment b(cfg);
    a.setSeed(42);
    b.setSeed(99);

    for (int i = 0; i < 600; ++i) {
        a.step(cfg.fixedTimestep);
        b.step(cfg.fixedTimestep);
    }

    bool different =
        (a.wind().x != b.wind().x || a.wind().y != b.wind().y || a.wind().z != b.wind().z);
    REQUIRE(different);
}

TEST_CASE("Environment difficulty ramps with time", "[Environment]") {
    GameConfig cfg;
    cfg.difficultyRamp = 0.1f;
    Environment env(cfg);
    REQUIRE(env.difficulty() == 1.0f);
    for (int i = 0; i < 600; ++i)
        env.step(cfg.fixedTimestep);
    REQUIRE(env.difficulty() > 1.5f);
}

TEST_CASE("Environment wind smoothing prevents jumps", "[Environment]") {
    GameConfig cfg;
    cfg.windSmoothing = 2.0f;
    Environment env(cfg);
    env.setSeed(7);

    Vec3 prevWind;
    for (int i = 0; i < 600; ++i) {
        env.step(cfg.fixedTimestep);
        Vec3 w = env.wind();
        // El cambio maximo en 1/60s esta limitado por el suavizado
        float change = (w - prevWind).length();
        REQUIRE(change < 10.0f);
        prevWind = w;
    }
}

TEST_CASE("Environment reset returns to initial state", "[Environment]") {
    GameConfig cfg;
    Environment env(cfg);
    env.setSeed(42);
    for (int i = 0; i < 1200; ++i)
        env.step(cfg.fixedTimestep);
    REQUIRE(env.difficulty() > 1.0f);

    env.reset();
    REQUIRE(env.difficulty() == 1.0f);
    // Despues de reset con misma semilla, debe ser determinista
    Environment fresh(cfg);
    fresh.setSeed(42);
    for (int i = 0; i < 60; ++i) {
        env.step(cfg.fixedTimestep);
        fresh.step(cfg.fixedTimestep);
    }
    REQUIRE(env.wind().x == fresh.wind().x);
}

// --- Generacion del escenario ---

namespace {

bool seSolapan(const drone::Obstacle& a, const drone::Obstacle& b) {
    const float holgura = 0.01f;  // el contacto justo no es solape
    return std::fabs(a.center.x - b.center.x) < (a.size.x + b.size.x) * 0.5f - holgura &&
           std::fabs(a.center.y - b.center.y) < (a.size.y + b.size.y) * 0.5f - holgura &&
           std::fabs(a.center.z - b.center.z) < (a.size.z + b.size.z) * 0.5f - holgura;
}

float radioEnPlanta(const drone::Obstacle& o) {
    return std::fmax(o.size.x, o.size.z) * 0.5f;
}

}  // namespace

TEST_CASE("Environment: cada entorno genera su propio escenario", "[Environment][escenario]") {
    GameConfig cfg;
    Environment ciudad(cfg), bosque(cfg), canon(cfg);
    ciudad.loadEnvironment("Ciudad Futurista");
    bosque.loadEnvironment("Bosque");
    canon.loadEnvironment("Canon");

    REQUIRE(ciudad.obstacles().size() > 10);
    REQUIRE(bosque.obstacles().size() > 10);
    REQUIRE(canon.obstacles().size() > 10);

    // Antes loadEnvironment ignoraba el nombre y los tres eran identicos.
    REQUIRE(ciudad.obstacles().size() != bosque.obstacles().size());
    REQUIRE(bosque.obstacles().size() != canon.obstacles().size());

    const auto cuenta = [](const Environment& e, drone::ObstacleKind k) {
        std::size_t n = 0;
        for (const drone::Obstacle& o : e.obstacles())
            n += (o.kind == k) ? 1 : 0;
        return n;
    };
    REQUIRE(cuenta(ciudad, drone::ObstacleKind::Building) > 0);
    REQUIRE(cuenta(bosque, drone::ObstacleKind::Trunk) > 0);
    // Cada arbol es tronco y copa: van siempre en pareja.
    REQUIRE(cuenta(bosque, drone::ObstacleKind::Canopy) ==
            cuenta(bosque, drone::ObstacleKind::Trunk));
    REQUIRE(cuenta(canon, drone::ObstacleKind::Rock) > 0);
}

TEST_CASE("Environment: un nombre desconocido tambien genera mapa", "[Environment][escenario]") {
    GameConfig cfg;
    Environment env(cfg);
    env.loadEnvironment("no existe este entorno");
    REQUIRE(env.obstacles().size() > 10);
    REQUIRE(env.landingZones().size() == 3);
}

TEST_CASE("Environment: el mismo nombre genera siempre el mismo mapa", "[Environment][escenario]") {
    GameConfig cfg;
    Environment a(cfg), b(cfg);
    a.loadEnvironment("Bosque");
    b.loadEnvironment("Bosque");

    REQUIRE(a.obstacles().size() == b.obstacles().size());
    for (std::size_t i = 0; i < a.obstacles().size(); ++i) {
        REQUIRE(a.obstacles()[i].center.x == b.obstacles()[i].center.x);
        REQUIRE(a.obstacles()[i].center.y == b.obstacles()[i].center.y);
        REQUIRE(a.obstacles()[i].center.z == b.obstacles()[i].center.z);
        REQUIRE(a.obstacles()[i].size.x == b.obstacles()[i].size.x);
        REQUIRE(a.obstacles()[i].kind == b.obstacles()[i].kind);
    }
}

// El escenario se siembra con el nombre, no con la semilla del viento: si
// dependiera de ella, setSeed() —que se llama DESPUES de cargar el entorno—
// dejaria a World con cuerpos de fisica de un mapa que ya no existe.
TEST_CASE("Environment: cambiar la semilla no mueve el escenario", "[Environment][escenario]") {
    GameConfig cfg;
    Environment env(cfg);
    env.loadEnvironment("Ciudad Futurista");
    const std::vector<drone::Obstacle> antes = env.obstacles();

    env.setSeed(12345);
    REQUIRE(env.obstacles().size() == antes.size());
    for (std::size_t i = 0; i < antes.size(); ++i)
        REQUIRE(env.obstacles()[i].center.x == antes[i].center.x);
}

TEST_CASE("Environment: el escenario deja libres despegue y plataformas",
          "[Environment][escenario]") {
    GameConfig cfg;
    for (const std::string nombre : {"Ciudad Futurista", "Bosque", "Canon"}) {
        Environment env(cfg);
        env.loadEnvironment(nombre);
        REQUIRE(env.landingZones().size() == 3);

        for (const drone::Obstacle& o : env.obstacles()) {
            const float r = radioEnPlanta(o);

            // Hueco para despegar sin comerse nada en el primer metro.
            const float distOrigen = std::sqrt(o.center.x * o.center.x + o.center.z * o.center.z);
            REQUIRE(distOrigen - r > 5.0f);

            // Las plataformas tienen que verse y poder entrarse.
            for (const Vec3& zona : env.landingZones()) {
                const float dx = o.center.x - zona.x, dz = o.center.z - zona.z;
                REQUIRE(std::sqrt(dx * dx + dz * dz) - r > 2.0f);
            }

            // Dentro del mundo y por debajo del techo, o no se podria pasar.
            REQUIRE(std::fabs(o.center.x) + o.size.x * 0.5f < cfg.worldHalfExtent);
            REQUIRE(std::fabs(o.center.z) + o.size.z * 0.5f < cfg.worldHalfExtent);
            REQUIRE(o.center.y + o.size.y * 0.5f <= cfg.maxAltitude);
            REQUIRE(o.center.y - o.size.y * 0.5f >= -0.01f);  // apoyado en el suelo
        }
    }
}

TEST_CASE("Environment: los obstaculos no se atraviesan entre si", "[Environment][escenario]") {
    GameConfig cfg;
    for (const std::string nombre : {"Ciudad Futurista", "Bosque", "Canon"}) {
        Environment env(cfg);
        env.loadEnvironment(nombre);
        const std::vector<drone::Obstacle>& obs = env.obstacles();
        for (std::size_t i = 0; i < obs.size(); ++i)
            for (std::size_t j = i + 1; j < obs.size(); ++j)
                REQUIRE_FALSE(seSolapan(obs[i], obs[j]));
    }
}
