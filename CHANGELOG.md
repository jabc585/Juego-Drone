# Changelog

Todas las versiones notables de Juego-Drone se documentarán en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.5.0] — 2026-08-04

Cierre de las Fases 1 y 2 del plan (PLAN2.md §17), salvo la tarea 2.6 (config TOML + spdlog), que queda pendiente.

### Added
- Física real en `PhysicsEngine` (R4): empuje, gravedad, arrastre/viento con velocidad terminal, suelo, límites del mundo, obstáculos AABB y consumo de batería proporcional al empuje.
- Bucle de timestep fijo a 60 Hz con acumulador e interpolación (R5, ADR-001).
- Entrada de terminal raw no bloqueante con termios/poll y soporte de flechas; fallback `conio` en Windows (ADR-007).
- Máquina de estados de juego: Playing/Paused/Settings/GameOver/ShuttingDown (§6.5); pausa y fin de partida funcionales.
- `Environment` con estado (R6): rachas de viento suavizadas con semilla determinista, dificultad progresiva y obstáculos.
- `EventBus` tipado (R7): BatteryLow/BatteryEmpty/Collision/LevelUp/DroneUnlocked, conectado a progresión y frontend.
- HUD ANSI repintado en sitio (§14.1): altitud, batería, viento, dificultad, nivel/XP, FPS y línea de avisos; salida plana sin TTY.
- `World` como agregado de simulación con snapshot inmutable para el frontend.
- Tests: física, EventBus, integración de `World` (invariantes, determinismo, colisiones) y humo E2E (`--version`, EOF limpio, comandos por pipe). 48 tests.
- CI: guard de arquitectura (sin I/O en core), formato clang-format fijado, matriz Debug/Release × 3 SO con `-Werror`, job de sanitizers (ASan+UBSan).
- `--version`/`--help` en el binario; regla de `install` y CPack (TGZ/ZIP).
- Dependabot y plantillas de issues.
- Estructura de proyecto profesional con separación core/frontend, Vec3, interfaces IRenderer/IInputSource, tests con Catch2 y CI multiplataforma.

### Changed
- Core migrado a `namespace drone`, C++17 y CMake ≥ 3.21 con warnings por target (R9).
- El core ya no contiene ninguna operación de I/O ni referencia al frontend concreto; composición por inyección en `main` (R3/D1).
- Progresión con umbral plano de 100 XP y excedente conservado (B7), multinivel por llamada.
- Convención de constantes `kPascalCase` en `.clang-tidy` alineada con docs/style.md.

### Fixed
- B1: eliminado el "¿Continuar? (s/n)" por frame; el juego corre en tiempo real.
- B2: el deltaTime ya no incluye la espera de teclado; física determinista por timestep fijo.
- B3: EOF/entrada inválida termina limpiamente (verificado por test de humo).
- B4: el dron ya no atraviesa el suelo; colisión con evento de impacto.
- B5: velocidad e inercia reales integradas por el motor de física.
- B6: el viento existe, se genera por rachas y afecta al vuelo.
- B8: el menú de pausa procesa sus opciones (reanudar/configuración/salir).
- B9: sin batería no hay empuje; batería agotada en suelo ⇒ fin de partida con reinicio.

## [0.1.0] — histórico

Prototipo original de consola (bucle bloqueante, clases stub).
