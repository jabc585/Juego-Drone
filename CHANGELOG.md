# Changelog

Todas las versiones notables de Juego-Drone se documentarán en este archivo.

El formato se basa en [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Modelo de vuelo de quadcopter en X portado de un controlador real: mezclador de 4 motores, PID angular con derivada sobre la medida, altitude hold, yaw hold, failsafe y trims. Todo en `drone_core` como funciones puras, con `World` como único puente a la física.
- Actitud del dron (roll/pitch/yaw) en `Drone` y en `WorldState`; el guardado sube a v2 con cuaternión y sigue aceptando saves v1.
- `applyLocalForceAtLocalPosition`, `applyLocalTorque` y `setInertiaTensor` en `PhysicsManager`, sin filtrar tipos de rp3d.
- Secciones `[drone.frame]`, `[drone.pid]` y `[drone.assist]` en `game.toml`: chasis, ganancias y asistencias configurables sin recompilar.
- 19 tests de mezclador, PID, holds, failsafe y trims, sin dependencia del motor de física. Suite total 97.
- `Command::Unknown` distingue «tecla sin acción» de «no se ha pulsado nada», que es lo que necesita Configuración para volver con cualquier tecla.
- 7 tests del parser de teclado de la terminal, inyectando bytes crudos por una tubería en stdin. Suite total 104.

### Added
- Escenarios generados por entorno en `Environment`. Antes `loadEnvironment()` ignoraba el nombre y soltaba siempre las mismas tres cajas: «Ciudad Futurista», «Bosque» y cualquier otro nombre daban el mismo mapa. Ahora hay ciudad de torres, bosque de árboles (tronco + copa, al estilo del ejemplo de raylib) y cañón de paredes y pilares, con el reparto sembrado a partir del nombre del entorno.
- `ObstacleKind` en `Obstacle`: la física los trata igual, pero el frontend gráfico ya no pinta un tronco del color de un rascacielos.

### Changed
- Las plataformas de aterrizaje las decide el entorno y no `World`. Estaban duplicadas en dos ficheros que tenían que decir lo mismo, y el generador de escenario podía plantar una torre encima.
- La elevación pasa a la barra espaciadora: una pulsación sube un metro, dos seguidas dejan el ascenso fijo y otra lo corta. Para bajar no hay tecla, se suelta y baja por gravedad. `Q`/`E` dejan de usarse.
- Las flechas mueven como `WASD` (antes ↑/↓ subían y bajaban), que es lo que anuncia la ayuda.

### Fixed
- El dron podía quedarse boca abajo para siempre. El casco es una esfera y al aterrizar rueda a cualquier postura; volcado, el empuje de los motores apunta al suelo y no despega ni con el mando a fondo. La partida se quedaba muerta sin llegar a «fin de partida». Ahora, posado y tumbado más de 32°, se corta el empuje y un par PD lo devuelve a la horizontal, con histéresis para no volver a volcarlo al soltar la ayuda.
- La actitud se extraía con `asin`, que no distingue 15° de 165°: un dron del revés se informaba como ligeramente inclinado, así que ni el HUD ni el PID se enteraban. Ahora se usa `atan2` contra la vertical del dron y cubre los 360°.
- F1–F4 cerraban el juego en Terminal.app, iTerm2 y xterm: esas teclas se mandan en SS3 (`ESC O P`), no en CSI, y el parser tomaba por Esc todo lo que no empezara por `ESC [`. Ahora se aceptan ambas formas.
- Cualquier secuencia de escape troceada entre dos lecturas cerraba la partida: los bytes que siguen a Esc se leían con espera cero, así que por ssh o tmux una flecha se interpretaba como Esc. Ahora se esperan hasta 100 ms a que la secuencia se complete.
- Configuración anunciaba «tecla para volver» pero solo respondía a las teclas con acción asignada; con cualquier otra la pantalla se quedaba encallada.
- El bucle principal giraba a ~650 FPS con una espera fija de 1 ms: 11 redibujados por paso de física y 344 KB/s de secuencias ANSI a la terminal. Limitado al ritmo del paso fijo (60 Hz), el consumo de CPU baja del 16,7 % al 2,5 %.
- La cabecera del HUD se desalineaba en Configuración: `%-14s` rellena por bytes y «CONFIGURACIÓN» ocupa 14 bytes en 13 columnas.
- `--help` no mencionaba H ni F1–F4, que el HUD sí ofrece.
- El dron no podía desplazarse en horizontal: el mezclador solo leía el eje vertical del mando y descartaba los cuatro controles horizontales. Ahora piden inclinación, que es como se traslada un quad.
- El empuje de cada motor se aplicaba con una fuerza en coordenadas de mundo sobre un punto local, así que el par dependía de dónde estuviera el dron en el mapa. Ahora fuerza y punto van en el marco del cuerpo, y el empuje sigue la inclinación del chasis.
- El PID angular recibía ceros constantes como actitud actual: creía que el dron estaba siempre nivelado y no estabilizaba nada.
- El término derivativo del cabeceo tenía el signo invertido —realimentaba en vez de amortiguar— y el dron volcaba a los pocos segundos de vuelo.
- El yaw no producía ningún giro: faltaba el par de reacción de las hélices, y las cuatro fuerzas paralelas no generan par sobre el eje vertical.
- `YawHold` no se llamaba nunca y `AltitudeHold` no tenía tecla asignada.
- Cada colisión se publicaba dos veces: una con la velocidad de impacto (m/s) y otra con el impulso (N·s) comparado contra `crashSpeed`, que está en m/s.
- Un retorno temprano por batería agotada se saltaba también el arrastre y el viento: el dron caía en el vacío.
- El integral del PID se acumulaba con el dron en el suelo y se liberaba de golpe al despegar.
- Las ganancias se copiaron del original sin convertir de grados a radianes, lo que las hacía ~35 veces más agresivas; el tope del integral (0,375) era mayor que la autoridad total del control (0,15).
- `hoverThrust` estaba fijado a una constante en vez de derivarse de la masa y la gravedad, así que cambiar la masa descuadraba el hover y el failsafe.
- El failsafe arrancaba el descenso desde cero en lugar de desde el empuje de hover, y se enganchaba estando el dron posado.
- El tensor de inercia era el de la esfera de colisión, unas 13 veces el de un quad real.
- `getNbOverlapPairs`/`getOverlapPair` no existen en rp3d 0.10.2: el manejador de triggers no compilaba.
- `EventType::LandingZone` sin tratar en tres `switch`, lo que rompía el build con `-Werror`.
- El cooldown de las zonas de aterrizaje era una variable `static` local, compartida por todas las instancias de `World`.
- El guard de CI de I/O en el core buscaba subcadenas: `bounciness` contiene "cin" y tumbaba el job por un falso positivo.
- `M_PI` es POSIX, no C++ estándar, y la matriz de CI compila también en Windows.


## [0.7.0] — 2026-08-05

### Added
- Motor de física ReactPhysics3D v0.10.2 en el módulo `drone_physics` (grafico.md, ADR-008 a ADR-013): `PhysicsManager` con pimpl, `HandlePool` con generaciones, `PhysicsSettings` desde `[physics.*]` del TOML. La física propia (`PhysicsEngine`) queda eliminada.
- `ContactBridge` (grafico.md §6.7): los contactos de rp3d se encolan dentro de `update()` y se despachan después, con velocidad de impacto e impulso estimados a partir de la velocidad previa al paso (§3.2 H2).
- `World::teleportDrone()` como único camino para mover el dron desde fuera de la física.
- Guards de CI de ADR-009: rp3d no aparece en `src/core`, `src/frontend`, `src/app` ni en la API pública de `drone_physics`.
- `docs/physics/CHANGELOG-fisica.md` (§11.3) y `docs/physics/baseline.md` con la línea base del motor.
- Suite de `HandlePool` sin dependencia de rp3d (§10.4) y test de que `PhysicsSettings` llega al motor. Suite total 78.
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
- `PhysicsManager::step()` no guardaba el transform previo de los cuerpos (el bucle estaba vacío): la interpolación tomaba como origen la posición de creación y la estimación de impulso trabajaba siempre con velocidad cero.
- No se publicaba ningún evento de colisión tras la migración: el dron no podía estrellarse, no había fin de partida por choque y el HUD nunca avisaba.
- `Drone::isGrounded()` comparaba la altura contra cero, pero la esfera simulada reposa sobre el suelo: daba siempre `false` y el fin de partida por batería agotada no llegaba nunca.
- `World::reset()` no reiniciaba los cuerpos de rp3d: al reiniciar, el dron reaparecía donde se había estrellado y con su velocidad.
- `applyLoad` perdía el nivel del jugador, disparaba una ráfaga de `LevelUp`, descartaba dificultad y tiempo de simulación, no acotaba el estado guardado y solo movía el estado de juego, así que cargar una partida no movía el dron.
- Los límites del mundo (techo y paredes) solo se aplicaban al estado de juego, que el siguiente paso sobrescribía desde el motor.
- Ciclo de enlace `drone_core` ↔ `drone_physics` y `drone_core` duplicado en la línea de enlace de la espiga.
- `loadPhysicsConfig` se tragaba en silencio cualquier error de parseo y `validatePhysicsConfig` truncaba sin avisar.
- `awakeCount()` devolvía el total de cuerpos en lugar de los despiertos.
- La clave `[physics] gravity` de `game.toml` ya no la leía nadie: se podía editar sin ningún efecto. La gravedad vive en `[physics.world]`.
- El HUD mostraba "Altitud: -0.0 m" con el dron posado, por la penetración residual del solver.
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
