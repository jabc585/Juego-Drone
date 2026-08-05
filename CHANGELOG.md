# Changelog

Todas las versiones notables de Juego-Drone se documentarán en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Frontend gráfico 3D con raylib (`--gui`): vista con cámara que sigue al dron, sombra proyectada, obstáculos del nivel, HUD y overlay de pausa/fin de partida. El core no se tocó para añadirlo, que era el criterio de aceptación de PLAN3 P0-5.
- `WorldState` expone la geometría del nivel (`obstacles`), de modo que el frontend dibuja exactamente lo que colisiona.
- Guardado/carga de partida (F5/F9 y autocarga al arrancar) en el directorio de datos del usuario, formato TOML versionado con validación estricta — se rechazan saves corruptos, con NaN/inf o con estados ilegales (PLAN3 P0-4).
- Configuración en runtime desde `assets/config/game.toml` (toml++): `GameConfig` inyectada, valores fuera de rango truncados con aviso (PLAN3 P0-2).
- Logging estructurado con spdlog en la capa app, suscrito al EventBus; el core sigue sin I/O (PLAN3 P0-1).
- Eventos `GameSaved`/`GameLoaded` con mensaje en el HUD y en el log.
- Tests de GameController, Environment, casos límite y geometría del snapshot; suite total 72 (PLAN3 P0-3, P2-2, P2-4).
- Restauración de la terminal ante SIGINT/SIGTERM/SIGHUP y salida anómala (PLAN3 P2-6).
- ADRs 004–007 y job de cobertura en CI.

### Fixed
- El modo terminal abría una ventana gráfica y volcaba los logs de raylib sobre el HUD: `main` construía todos los frontends, y los constructores tienen efectos colaterales (raw mode, ventana GL). Ahora solo se construye el elegido.
- En modo gráfico, `RaylibInput` releía el teclado en cada una de las hasta 32 llamadas a `poll()` por frame: una pulsación de `P` alternaba la pausa 32 veces (efecto neto: nada) y `F5` guardaba 32 veces. Ahora el estado del teclado se lee una vez por frame y se sirve desde una cola.
- En modo gráfico, mantener una tecla de movimiento impedía por completo pausar, guardar o reiniciar: `poll()` devolvía en el primer `if` y nunca llegaba a las teclas de acción.
- El renderer gráfico dibujaba una copia hardcodeada de los obstáculos, que podía divergir de los que simula la física.
- La cámara era fija: el dron se perdía de vista en cuanto se alejaba del origen (el mundo mide 200 m de lado).
- En modo gráfico no había ninguna indicación de pausa ni de fin de partida; la escena simplemente se congelaba.
- `SetTargetFPS(0)` dejaba el render sin límite; ahora 60 FPS. Los ~30 logs INFO de raylib al arrancar se silencian.
- El frontend de raylib se compilaba sin los warnings del proyecto; ahora enlaza `drone_warnings`.
- El bucle de `main` llamaba `tick(0.0f)` sin reloj ni sleep: el mundo no avanzaba nunca y la CPU iba al 100 %; ahora mide el tiempo real de frame como `GameController::run()`.
- `saveGame` desreferenciaba un puntero nulo al construir el TOML (crash al guardar) y nunca creaba el directorio de guardado (el primer guardado fallaba siempre).
- Un `game.toml` o `save.toml` malformado abortaba el proceso en builds Debug por una aserción interna de toml++; ahora toda entrada inválida degrada a aviso + defaults (`app/TomlSafe.h`).
- `applyLoad` perdía el nivel del jugador (solo re-aplicaba la XP residual), disparaba una ráfaga de eventos LevelUp al cargar y descartaba dificultad/tiempo de simulación; un save con estado ilegal podía cerrar el juego al cargarse.
- El mensaje de "partida guardada" reutilizaba el evento LevelUp y mostraba "¡Nivel 0 alcanzado!".
- Solo se registraba SIGTERM: Ctrl+C (SIGINT) dejaba la terminal en modo raw sin eco.
- `DRONE_VERSION` se perdió del build: el binario y el log reportaban 0.0.0-dev.
- spdlog fijado a v1.14.1 y dependencias marcadas SYSTEM (v1.13 no compila con `-Werror` en libc++ moderno).
- Tests de GameOver reescritos: asumían que `tick(10.0f)` simula 10 s (el clamp de `maxFrameTime` lo limita a 0.25 s) y que un dron posado en el suelo puede estrellarse.

## [0.5.0] — 2026-08-04

Cierre de las Fases 1 y 2 del plan original (PLAN3.md §17 documenta la trazabilidad), salvo la tarea 2.6 (config TOML + spdlog), que queda pendiente.

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
