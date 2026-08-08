#include "frontend/raylib/ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace drone {

void ParticleSystem::emit(const Vector3& pos, int count, float spread, float speed, float life,
                          float size, Color color) {
    for (int i = 0; i < count; ++i) {
        float ax = (rand() % 1000 - 500) / 500.0f;
        float ay = (rand() % 1000) / 1000.0f;
        float az = (rand() % 1000 - 500) / 500.0f;
        Particle p;
        p.position = pos;
        p.velocity = {ax * spread * speed, ay * speed * 2.0f, az * spread * speed};
        p.life = life * (0.5f + (rand() % 500) / 1000.0f);
        p.size = size * (0.5f + (rand() % 500) / 1000.0f);
        p.color = color;
        m_particles.push_back(p);
    }
}

void ParticleSystem::emitRing(const Vector3& pos, int count, float radius, float speed, float life,
                              float size, Color color) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 6283) / 1000.0f;
        float r = radius * (0.8f + (rand() % 400) / 1000.0f);
        Particle p;
        p.position = {pos.x + cosf(angle) * r, pos.y, pos.z + sinf(angle) * r};
        p.velocity = {cosf(angle) * speed * 0.5f, speed * 0.3f, sinf(angle) * speed * 0.5f};
        p.life = life * (0.5f + (rand() % 500) / 1000.0f);
        p.size = size;
        p.color = color;
        m_particles.push_back(p);
    }
}

void ParticleSystem::update(float dt) {
    for (auto& p : m_particles) {
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;
        p.velocity.y -= 2.0f * dt;
        p.life -= dt;
        p.size *= 0.995f;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                                     [](const Particle& p) { return p.life <= 0; }),
                      m_particles.end());
}

void ParticleSystem::draw() const {
    for (const auto& p : m_particles) {
        Color c = p.color;
        c.a = particleAlpha(p.life);
        DrawCube(p.position, p.size, p.size, p.size, c);
    }
}

}  // namespace drone
