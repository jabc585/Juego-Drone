#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "core/GameConfig.h"
#include "core/World.h"
#include "physics/PhysicsSettings.h"

using drone::Event;
using drone::EventType;
using drone::GameConfig;
using drone::Vec3;
using drone::World;
using drone::WorldState;
using drone::physics::PhysicsSettings;

namespace {

void run(World& w, int steps, const GameConfig& cfg) {
    for (int i = 0; i < steps; ++i)
        w.step(cfg.fixedTimestep);
}

// rp3d deja una penetración residual en el contacto de reposo: el suelo no
// es una barrera dura como en la física propia, sino un contacto que el
// solver corrige en unas pocas iteraciones.
constexpr float kPenetrationSlack = 0.05f;

}  // namespace

TEST_CASE("World: invariants hold over a full accelerated flight", "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    World w(cfg, physCfg);
    w.loadEnvironment("Ciudad Futurista");
    w.environment().setSeed(1234);

    for (int second = 0; second < 60; ++second) {
        const float dir = (second % 2 == 0) ? 1.0f : -0.3f;
        w.setThrustInput({0.4f * dir, 0.8f, 0.4f});
        for (int i = 0; i < 60; ++i) {
            w.step(cfg.fixedTimestep);
            const Vec3 p = w.drone().position();
            REQUIRE(p.y >= -kPenetrationSlack);
            REQUIRE(p.y <= cfg.maxAltitude);
            REQUIRE(std::fabs(p.x) <= cfg.worldHalfExtent);
            REQUIRE(std::fabs(p.z) <= cfg.worldHalfExtent);
            REQUIRE(w.drone().battery() >= 0.0f);
            REQUIRE(w.drone().battery() <= cfg.batteryMax);
        }
    }
}

TEST_CASE("World: same seed and commands produce identical trajectory", "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    auto simulate = [&]() {
        World w(cfg, physCfg);
        w.loadEnvironment("Ciudad Futurista");
        w.environment().setSeed(42);
        w.setThrustInput({0.3f, 0.7f, 0.5f});
        for (int i = 0; i < 1800; ++i)
            w.step(cfg.fixedTimestep);
        return w.drone().position();
    };
    const Vec3 a = simulate();
    const Vec3 b = simulate();
    REQUIRE(a.x == b.x);
    REQUIRE(a.y == b.y);
    REQUIRE(a.z == b.z);
}

TEST_CASE("World: wind gusts appear over time (B6 closed)", "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    World w(cfg, physCfg);
    w.environment().setSeed(7);
    float maxWind = 0.0f;
    for (int i = 0; i < 60 * 30; ++i) {
        w.step(cfg.fixedTimestep);
        maxWind = std::max(maxWind, w.environment().wind().length());
    }
    REQUIRE(maxWind > 0.5f);
}

TEST_CASE("World: difficulty ramps with time (D3 closed)", "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    World w(cfg, physCfg);
    REQUIRE(w.environment().difficulty() == 1.0f);
    run(w, 60 * 60, cfg);
    REQUIRE(w.environment().difficulty() > 1.5f);
}

TEST_CASE("World: obstacle collision pushes the drone out and notifies", "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    World w(cfg, physCfg);
    w.loadEnvironment("Ciudad Futurista");
    int collisions = 0;
    w.events().subscribe(EventType::Collision, [&](const Event&) { ++collisions; });

    // Contra un obstaculo REAL del mapa generado, no contra unas coordenadas
    // fijas: el escenario se genera por entorno y clavar posiciones aqui
    // ataba el test a un reparto de edificios concreto.
    REQUIRE_FALSE(w.environment().obstacles().empty());
    const drone::Obstacle& blanco = w.environment().obstacles().front();

    // Por teleportDrone: escribir en Drone::setPosition solo tocaba el
    // estado de juego y el paso siguiente lo sobrescribia desde rp3d, asi
    // que el dron nunca llegaba a acercarse al edificio.
    const float aparte = blanco.size.z * 0.5f + cfg.droneRadius + 3.0f;
    w.teleportDrone({blanco.center.x, blanco.center.y, blanco.center.z - aparte},
                    {0.0f, 0.0f, 6.0f});
    w.setThrustInput({0, 0.6f, 0.8f});
    run(w, 240, cfg);

    REQUIRE(collisions > 0);

    // El dron queda fuera del edificio. La holgura descuenta la penetracion
    // residual del solver: rp3d no expulsa a la superficie exacta como hacia
    // el push-out de la fisica propia, deja decimas de milimetro de solape.
    const Vec3 p = w.drone().position();
    const bool insideX = std::fabs(p.x - blanco.center.x) <
                         blanco.size.x * 0.5f + cfg.droneRadius - kPenetrationSlack;
    const bool insideY = std::fabs(p.y - blanco.center.y) <
                         blanco.size.y * 0.5f + cfg.droneRadius - kPenetrationSlack;
    const bool insideZ = std::fabs(p.z - blanco.center.z) <
                         blanco.size.z * 0.5f + cfg.droneRadius - kPenetrationSlack;
    REQUIRE_FALSE((insideX && insideY && insideZ));
}

TEST_CASE("World: snapshot exposes the same obstacles the physics collides with",
          "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    World w(cfg, physCfg);
    // Sin nivel cargado no hay geometría que dibujar.
    REQUIRE(w.snapshot().obstacles.empty());

    w.loadEnvironment("Ciudad Futurista");
    const WorldState s = w.snapshot();

    // El frontend debe dibujar exactamente lo que el motor colisiona: si el
    // renderer usa su propia copia, la escena miente sobre el mundo real.
    REQUIRE(s.obstacles.size() == w.environment().obstacles().size());
    REQUIRE_FALSE(s.obstacles.empty());
    for (std::size_t i = 0; i < s.obstacles.size(); ++i) {
        const drone::Obstacle& expected = w.environment().obstacles()[i];
        REQUIRE(s.obstacles[i].center.x == expected.center.x);
        REQUIRE(s.obstacles[i].center.y == expected.center.y);
        REQUIRE(s.obstacles[i].center.z == expected.center.z);
        REQUIRE(s.obstacles[i].size.x == expected.size.x);
        REQUIRE(s.obstacles[i].size.y == expected.size.y);
        REQUIRE(s.obstacles[i].size.z == expected.size.z);
    }
}

TEST_CASE("World: reset restores a fresh deterministic world", "[World][integration]") {
    GameConfig cfg;
    PhysicsSettings physCfg;
    World w(cfg, physCfg);
    w.loadEnvironment("Ciudad Futurista");
    w.environment().setSeed(99);
    w.setThrustInput({0.5f, 1.0f, 0.2f});
    run(w, 600, cfg);
    REQUIRE(w.simTime() > 9.9f);

    w.reset();
    REQUIRE(w.simTime() == 0.0f);
    REQUIRE(w.drone().position().length() == 0.0f);
    REQUIRE(w.drone().battery() == cfg.batteryMax);
    REQUIRE(w.environment().difficulty() == 1.0f);
}

// El casco es una esfera: al aterrizar rueda y puede quedarse del reves. Con
// el empuje apuntando al suelo, el dron no despegaba nunca mas y la partida
// se quedaba muerta sin llegar a "fin de partida".
TEST_CASE("World: un dron volcado se endereza y vuelve a despegar", "[World][enderezado]") {
    drone::physics::PhysicsSettings physCfg;
    drone::GameConfig cfg;
    cfg.batteryPerNewton = 0.0f;
    drone::World w(cfg, physCfg);

    const auto arribaDelDron = [&w] {
        const drone::WorldState s = w.snapshot();
        return 1.0f - 2.0f * (s.droneQx * s.droneQx + s.droneQz * s.droneQz);
    };

    w.teleportDrone({0.0f, 0.0f, 0.0f});
    w.setDroneOrientation(0.0f, 0.0f, 1.0f, 0.0f);  // 180 grados: boca abajo
    w.step(cfg.fixedTimestep);
    REQUIRE(arribaDelDron() < -0.9f);

    // Con el mando a fondo, que es lo que haria el jugador al ver que no sube.
    w.setThrustInput({0.0f, 1.0f, 0.0f});
    for (int i = 0; i < 60 * 5; ++i)
        w.step(cfg.fixedTimestep);

    REQUIRE(arribaDelDron() > 0.5f);         // se ha enderezado
    REQUIRE(w.drone().position().y > 1.0f);  // y ha podido despegar
}
