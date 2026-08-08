#include "core/Environment.h"

#include <algorithm>
#include <cmath>

namespace drone {

namespace {

// Distancia libre alrededor del punto de aparición: el dron tiene que poder
// despegar sin comerse un edificio en el primer metro.
constexpr float kSpawnClearance = 7.0f;

// Margen extra alrededor de una zona de aterrizaje, para que se vea y se
// pueda entrar en ella.
constexpr float kZoneClearance = 4.0f;

// Separación mínima entre dos obstáculos. Pegados, la física los trata como
// una pared y al jugador le parecen un solo bloque.
constexpr float kObstacleGap = 1.5f;

// El mapa útil es menor que el límite del mundo: los bordes se dejan vacíos
// para que quede sitio donde recuperarse.
constexpr float kFieldMargin = 0.75f;

// FNV-1a. El escenario se siembra con el NOMBRE del entorno, no con la
// semilla del viento: setSeed() se llama después de loadEnvironment(), y si
// el escenario dependiera de ella habría que regenerarlo — dejando a World
// con cuerpos de física de un mapa que ya no existe.
uint32_t hashName(const std::string& name) {
    uint32_t h = 2166136261u;
    for (const char c : name) {
        h ^= static_cast<uint32_t>(static_cast<unsigned char>(c));
        h *= 16777619u;
    }
    return h;
}

// Reparte cajas por el mapa respetando los huecos que deben quedar libres.
class Scatter {
public:
    Scatter(std::vector<Obstacle>& out, const std::vector<Vec3>& zones, float halfExtent,
            float maxHeight, uint32_t seed)
        : m_out(out),
          m_zones(zones),
          m_limit(halfExtent * kFieldMargin),
          m_maxHeight(maxHeight),
          m_rng(seed) {}

    float range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(m_rng); }

    bool chance(float probability) { return range(0.0f, 1.0f) < probability; }

    float limit() const { return m_limit; }

    // Coloca una caja apoyada en el suelo, si el sitio está libre.
    bool place(float x, float z, float width, float height, float depth, ObstacleKind kind) {
        const float top = std::min(height, m_maxHeight);
        const Vec3 size{width, top, depth};
        const Vec3 center{x, top * 0.5f, z};
        if (!fits(center, size))
            return false;
        m_out.push_back({center, size, kind});
        return true;
    }

    // Un árbol se reserva por la copa, que es su parte ancha. Midiendo solo
    // el tronco cabían dos a dos metros y las copas se atravesaban.
    bool placeTree(float x, float z, float trunkSide, float trunkHeight, float canopy) {
        const Vec3 footprint{canopy, trunkHeight + canopy, canopy};
        if (!fits({x, footprint.y * 0.5f, z}, footprint))
            return false;
        m_out.push_back(
            {{x, trunkHeight * 0.5f, z}, {trunkSide, trunkHeight, trunkSide}, ObstacleKind::Trunk});
        m_out.push_back(
            {{x, trunkHeight + canopy * 0.5f, z}, {canopy, canopy, canopy}, ObstacleKind::Canopy});
        return true;
    }

private:
    bool fits(const Vec3& center, const Vec3& size) const {
        if (std::fabs(center.x) + size.x * 0.5f > m_limit ||
            std::fabs(center.z) + size.z * 0.5f > m_limit)
            return false;

        const float reach = std::max(size.x, size.z) * 0.5f;
        if (std::sqrt(center.x * center.x + center.z * center.z) < kSpawnClearance + reach)
            return false;

        for (const Vec3& zone : m_zones) {
            const float dx = center.x - zone.x, dz = center.z - zone.z;
            if (std::sqrt(dx * dx + dz * dz) < kZoneClearance + reach)
                return false;
        }

        for (const Obstacle& o : m_out) {
            if (std::fabs(o.center.x - center.x) < (o.size.x + size.x) * 0.5f + kObstacleGap &&
                std::fabs(o.center.z - center.z) < (o.size.z + size.z) * 0.5f + kObstacleGap)
                return false;
        }
        return true;
    }

    std::vector<Obstacle>& m_out;
    const std::vector<Vec3>& m_zones;
    float m_limit;
    float m_maxHeight;
    std::mt19937 m_rng;
};

// Torres de alturas dispares en una retícula descolocada, para que se vean
// calles pero no un tablero de ajedrez.
void buildCity(Scatter& s) {
    const float spacing = 14.0f;
    for (float x = -s.limit(); x <= s.limit(); x += spacing) {
        for (float z = -s.limit(); z <= s.limit(); z += spacing) {
            if (!s.chance(0.75f))
                continue;
            const float side = s.range(3.0f, 7.0f);
            s.place(x + s.range(-3.5f, 3.5f), z + s.range(-3.5f, 3.5f), side, s.range(8.0f, 42.0f),
                    side * s.range(0.7f, 1.4f), ObstacleKind::Building);
        }
    }
}

// Tronco + copa, como los "árboles" de cubos del ejemplo de raylib, pero
// con altura y porte variables.
void buildForest(Scatter& s) {
    const float spacing = 10.0f;
    for (float x = -s.limit(); x <= s.limit(); x += spacing) {
        for (float z = -s.limit(); z <= s.limit(); z += spacing) {
            if (!s.chance(0.62f))
                continue;
            s.placeTree(x + s.range(-3.0f, 3.0f), z + s.range(-3.0f, 3.0f), s.range(0.4f, 0.7f),
                        s.range(2.0f, 4.5f), s.range(2.5f, 4.5f));
        }
    }
}

// Dos paredes largas con un pasillo en medio y pilares sueltos dentro.
void buildCanyon(Scatter& s) {
    const float wallOffset = 22.0f;
    for (float z = -s.limit(); z <= s.limit(); z += 12.0f) {
        for (const float side : {-1.0f, 1.0f}) {
            s.place(side * (wallOffset + s.range(-2.0f, 2.0f)), z + s.range(-2.0f, 2.0f),
                    s.range(6.0f, 10.0f), s.range(12.0f, 34.0f), 10.0f, ObstacleKind::Rock);
        }
    }
    for (int i = 0; i < 40; ++i) {
        const float side = s.range(2.0f, 6.0f);
        s.place(s.range(-s.limit(), s.limit()), s.range(-s.limit(), s.limit()), side,
                s.range(4.0f, 16.0f), side, ObstacleKind::Rock);
    }
}

}  // namespace

Environment::Environment(const GameConfig& cfg) : m_config(cfg), m_rng(0) {
    scheduleNextGust();
}

void Environment::loadEnvironment(const std::string& environmentName) {
    m_name = environmentName;
    m_obstacles.clear();

    // Las plataformas van antes que los obstáculos: el generador las esquiva.
    m_landingZones = landingZonesFor(environmentName);

    Scatter scatter(m_obstacles, m_landingZones, m_config.worldHalfExtent, m_config.maxAltitude,
                    hashName(environmentName));

    if (environmentName == "Bosque") {
        buildForest(scatter);
    } else if (environmentName == "Cañón" || environmentName == "Canon") {
        buildCanyon(scatter);
    } else {
        // Ciudad Futurista y cualquier nombre no reconocido.
        buildCity(scatter);
    }
}

std::vector<Vec3> Environment::landingZonesFor(const std::string& environmentName) const {
    const float h = m_config.landingZoneHeight;
    if (environmentName == "Bosque")
        return {{18.0f, h, -14.0f}, {-16.0f, h, 20.0f}, {-24.0f, h, -22.0f}};
    if (environmentName == "Cañón" || environmentName == "Canon")
        return {{0.0f, h, 26.0f}, {0.0f, h, -30.0f}, {12.0f, h, 0.0f}};
    return {{15.0f, h, 15.0f}, {-20.0f, h, -10.0f}, {5.0f, h, -25.0f}};
}

void Environment::setSeed(uint32_t seed) {
    m_seed = seed;
    m_rng.seed(seed);
    scheduleNextGust();
}

void Environment::scheduleNextGust() {
    std::uniform_real_distribution<float> interval(m_config.gustMinInterval,
                                                   m_config.gustMaxInterval);
    m_timeToNextGust = interval(m_rng);
}

void Environment::step(float dt) {
    m_elapsed += dt;
    m_difficulty = std::min(m_config.maxDifficulty, 1.0f + m_elapsed * m_config.difficultyRamp);

    m_timeToNextGust -= dt;
    if (m_timeToNextGust <= 0.0f) {
        std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
        std::uniform_real_distribution<float> magDist(0.5f, 1.5f);
        std::uniform_real_distribution<float> vertDist(-0.1f, 0.1f);
        const float angle = angleDist(m_rng);
        const float magnitude = magDist(m_rng) * m_difficulty * m_config.windBaseSpeed;
        m_windTarget = {std::cos(angle) * magnitude, vertDist(m_rng) * magnitude,
                        std::sin(angle) * magnitude};
        scheduleNextGust();
    }

    const float blend = std::min(1.0f, m_config.windSmoothing * dt);
    m_wind += (m_windTarget - m_wind) * blend;
}

void Environment::restoreProgress(float elapsed) {
    m_elapsed = std::max(0.0f, elapsed);
    m_difficulty = std::min(m_config.maxDifficulty, 1.0f + m_elapsed * m_config.difficultyRamp);
}

void Environment::reset() {
    m_wind = {};
    m_windTarget = {};
    m_difficulty = 1.0f;
    m_elapsed = 0.0f;
    m_rng.seed(m_seed);
    scheduleNextGust();
}

}  // namespace drone
