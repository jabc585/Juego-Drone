// Unitarios sin rp3d (grafico.md §10.4, criterio de aceptación de la Fase 1).
// Estos tests no enlazan el motor: se ejecutan en milisegundos y cubren el
// mecanismo del que dependen todos los BodyId.
#include <catch2/catch_test_macros.hpp>

#include "physics/HandlePool.h"

using drone::physics::HandlePool;

TEST_CASE("HandlePool: a fresh handle resolves to its value", "[HandlePool]") {
    HandlePool<int> pool;
    const auto h = pool.create(42);

    REQUIRE(h.valid());
    REQUIRE(pool.valid(h));
    REQUIRE(pool.get(h) != nullptr);
    REQUIRE(*pool.get(h) == 42);
    REQUIRE(pool.aliveCount() == 1);
}

TEST_CASE("HandlePool: a default handle is never valid", "[HandlePool]") {
    HandlePool<int> pool;
    pool.create(1);

    // Generación 0 es el handle nulo: aunque el índice 0 exista y esté vivo.
    const HandlePool<int>::Handle null{};
    REQUIRE_FALSE(null.valid());
    REQUIRE_FALSE(pool.valid(null));
    REQUIRE(pool.get(null) == nullptr);
}

TEST_CASE("HandlePool: destroying invalidates the handle", "[HandlePool]") {
    HandlePool<int> pool;
    const auto h = pool.create(7);
    pool.destroy(h);

    REQUIRE_FALSE(pool.valid(h));
    REQUIRE(pool.get(h) == nullptr);
    REQUIRE(pool.aliveCount() == 0);
}

TEST_CASE("HandlePool: a stale handle does not reach the body that reused its slot",
          "[HandlePool]") {
    HandlePool<int> pool;
    const auto old = pool.create(100);
    pool.destroy(old);
    const auto fresh = pool.create(200);

    // El índice se reutiliza — ese es el objetivo del pooling…
    REQUIRE(fresh.index == old.index);
    // …pero la generación cambia, así que el handle viejo no lo alcanza.
    REQUIRE(fresh.generation != old.generation);
    REQUIRE(pool.get(old) == nullptr);
    REQUIRE(*pool.get(fresh) == 200);
}

TEST_CASE("HandlePool: destroying twice is a no-op, not a corrupted free list", "[HandlePool]") {
    HandlePool<int> pool;
    const auto h = pool.create(1);
    pool.destroy(h);
    pool.destroy(h);

    REQUIRE(pool.aliveCount() == 0);
    // Si el segundo destroy hubiese encolado el índice otra vez, dos create
    // devolverían el mismo índice y un cuerpo pisaría al otro.
    const auto a = pool.create(10);
    const auto b = pool.create(20);
    REQUIRE(a.index != b.index);
}

TEST_CASE("HandlePool: forEach only visits live entries", "[HandlePool]") {
    HandlePool<int> pool;
    const auto a = pool.create(1);
    const auto b = pool.create(2);
    const auto c = pool.create(3);
    pool.destroy(b);

    int visited = 0;
    int sum = 0;
    pool.forEach([&](HandlePool<int>::Handle h, int& value) {
        ++visited;
        sum += value;
        REQUIRE(pool.valid(h));
    });

    REQUIRE(visited == 2);
    REQUIRE(sum == 4);
    REQUIRE(pool.valid(a));
    REQUIRE(pool.valid(c));
}

TEST_CASE("HandlePool: handles survive the growth of the backing vector", "[HandlePool]") {
    HandlePool<int> pool;
    const auto first = pool.create(0);
    for (int i = 1; i < 1000; ++i)
        pool.create(i);

    // El vector interno ha realojado varias veces por el camino; el handle
    // sigue resolviendo porque no guarda punteros, solo índice y generación.
    REQUIRE(pool.valid(first));
    REQUIRE(*pool.get(first) == 0);
    REQUIRE(pool.aliveCount() == 1000);
}
