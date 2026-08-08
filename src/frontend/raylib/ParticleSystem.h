#pragma once

#include <raylib.h>

#include <vector>

namespace drone {

// El alfa se acota ANTES de convertir. Las emisiones de despegue usan vidas
// por encima de 1, y life*255 se salia del rango de unsigned char: eso es
// comportamiento indefinido, y UBSan lo cazaba en el primer frame.
inline unsigned char particleAlpha(float life) {
    const float a = life * 255.0f;
    return static_cast<unsigned char>(a < 0.0f ? 0.0f : (a > 255.0f ? 255.0f : a));
}

struct Particle {
    Vector3 position;
    Vector3 velocity;
    float life = 1.0f;
    float size = 0.1f;
    Color color = WHITE;
};

class ParticleSystem {
public:
    void emit(const Vector3& pos, int count, float spread, float speed, float life, float size,
              Color color);
    void emitRing(const Vector3& pos, int count, float radius, float speed, float life, float size,
                  Color color);
    void update(float dt);
    void draw() const;
    size_t count() const { return m_particles.size(); }

private:
    std::vector<Particle> m_particles;
};

}  // namespace drone
