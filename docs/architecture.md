# Arquitectura — Juego-Drone

## Principio

Separación estricta entre **core** (librería de simulación, sin I/O) y **frontend** (capa de presentación intercambiable). El core define el contrato con DTOs (`WorldState`, `Command`, `Event`); el frontend lo implementa. La CI verifica con un guard (`grep`) que `src/core/` no contiene `cout`/`cin`/`printf`.

```
src/app/main.cpp                    (composición e inyección)
  ├── TerminalInput  ──implementa──> IInputSource ─┐
  ├── TerminalRenderer ─implementa─> IRenderer ────┤
  └── GameController                               │ (interfaces puras,
        ├── máquina de estados (GameState)         │  sin I/O)
        ├── bucle de timestep fijo 60 Hz (ADR-001) │
        ├── World                                  │
        │     ├── Drone         (estado)           │
        │     ├── Environment   (viento, dificultad, obstáculos)
        │     ├── PhysicsEngine (única integración de fuerzas)
        │     └── EventBus      (BatteryLow, Collision, LevelUp…)
        └── PlayerProgression   (publica LevelUp/DroneUnlocked)
```

## Regla de dependencias

- `core/` puede incluir las **interfaces** de `frontend/` (`IRenderer.h`, `IInputSource.h`) — son puras y sin I/O.
- `core/` **nunca** incluye `frontend/terminal/` ni ninguna implementación concreta.
- `frontend/` conoce los DTOs del core (`WorldState`, `Command`, `Event`, `GameState`).
- Solo `app/` conoce a ambos lados: construye e inyecta.

## Flujo de un frame (timestep fijo, ADR-001)

1. `IInputSource::poll()` — no bloquea; devuelve `Command::None` sin entrada, `Quit` en EOF.
2. `GameController::handleCommand()` — transiciones de la máquina de estados y pulsos de empuje.
3. Mientras el acumulador ≥ 1/60 s: `World::step(dt)` → `Environment::step` (viento/dificultad) → `PhysicsEngine::step` (fuerzas, colisiones, batería) → eventos por `EventBus`.
4. `IRenderer::draw(snapshot, alpha)` — el frontend pinta el estado interpolable.

## Física (`PhysicsEngine::step`)

- Fuerzas: empuje (si hay batería) + gravedad + término aerodinámico único `kDrag · (viento − velocidad)` que da a la vez arrastre, velocidad terminal y empuje de rachas.
- Integración Euler semi-implícita (v después p), estable a 60 Hz.
- Colisiones: suelo (evento si el impacto supera `kCrashSpeed`), límites del mundo, obstáculos AABB por eje de mínima penetración.
- Batería: consumo ∝ |empuje|·dt; eventos `BatteryLow`/`BatteryEmpty` al cruzar umbrales.

## Determinismo

`Environment::setSeed()` fija la secuencia de rachas; con timestep fijo, misma semilla + mismos comandos ⇒ misma trayectoria (verificado por test de integración). Todas las constantes viven en `core/Config.h` (pendiente tarea 2.6: externalizar a TOML).
