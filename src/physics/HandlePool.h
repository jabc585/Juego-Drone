#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace drone::physics {

// Pool de handles con generación (grafico.md §6.2, ADR-010).
//
// Un handle liberado y reutilizado incrementa su generación, así que un id
// antiguo NO apunta al objeto nuevo: get() devuelve nullptr en vez de un
// puntero a otra cosa. Es la diferencia entre un bug detectable y una
// corrupción silenciosa.
template <typename T>
class HandlePool {
public:
    struct Handle {
        uint32_t index = 0;
        uint32_t generation = 0;
        bool valid() const { return generation != 0; }
    };

    Handle create(T value);
    bool valid(Handle h) const;
    T* get(Handle h);
    const T* get(Handle h) const;
    void destroy(Handle h);
    size_t aliveCount() const;

    // Recorre solo los elementos vivos. Lo necesita captureTransforms()
    // (§6.4): sin iteración no hay forma de guardar el transform previo de
    // cada cuerpo y la interpolación se queda sin origen.
    template <typename F>
    void forEach(F&& fn);
    template <typename F>
    void forEach(F&& fn) const;

private:
    std::vector<T> m_items;
    std::vector<uint32_t> m_generations;
    std::vector<uint8_t> m_alive;
    std::vector<uint32_t> m_freeList;
};

template <typename T>
typename HandlePool<T>::Handle HandlePool<T>::create(T value) {
    uint32_t index;
    if (!m_freeList.empty()) {
        index = m_freeList.back();
        m_freeList.pop_back();
        m_items[index] = std::move(value);
    } else {
        index = static_cast<uint32_t>(m_items.size());
        m_items.push_back(std::move(value));
        m_generations.push_back(1);
        m_alive.push_back(0);
    }
    // La generación 0 se reserva para el handle nulo: al desbordar se salta.
    if (m_generations[index] == 0)
        m_generations[index] = 1;
    m_alive[index] = 1;
    return {index, m_generations[index]};
}

template <typename T>
bool HandlePool<T>::valid(Handle h) const {
    return h.generation != 0 && h.index < m_items.size() && m_alive[h.index] != 0 &&
           m_generations[h.index] == h.generation;
}

template <typename T>
T* HandlePool<T>::get(Handle h) {
    return valid(h) ? &m_items[h.index] : nullptr;
}

template <typename T>
const T* HandlePool<T>::get(Handle h) const {
    return valid(h) ? &m_items[h.index] : nullptr;
}

template <typename T>
void HandlePool<T>::destroy(Handle h) {
    if (!valid(h))
        return;
    ++m_generations[h.index];
    m_alive[h.index] = 0;
    m_freeList.push_back(h.index);
}

template <typename T>
size_t HandlePool<T>::aliveCount() const {
    return m_items.size() - m_freeList.size();
}

template <typename T>
template <typename F>
void HandlePool<T>::forEach(F&& fn) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_items.size()); ++i) {
        if (m_alive[i] != 0)
            fn(Handle{i, m_generations[i]}, m_items[i]);
    }
}

template <typename T>
template <typename F>
void HandlePool<T>::forEach(F&& fn) const {
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_items.size()); ++i) {
        if (m_alive[i] != 0)
            fn(Handle{i, m_generations[i]}, m_items[i]);
    }
}

}  // namespace drone::physics
