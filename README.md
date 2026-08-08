# Juego-Drone

[![CI](https://github.com/jabc585/Juego-Drone/actions/workflows/ci.yml/badge.svg)](https://github.com/jabc585/Juego-Drone/actions/workflows/ci.yml)

Simulador de vuelo de dron en tiempo real: física con empuje, gravedad, arrastre y rachas de viento; batería; obstáculos; progresión por niveles y guardado de partida. Dos frontends sobre el mismo motor: HUD ANSI en terminal y vista 3D con raylib.

## Requisitos

- CMake ≥ 3.21
- Compilador C++17 (Clang, GCC o MSVC)
- Opcional: [raylib](https://www.raylib.com/) para el modo gráfico (`brew install raylib`). Si no está instalada, el proyecto compila igual y solo ofrece el modo terminal.

## Compilación y ejecución

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

./build/src/app/DroneFlightSim          # modo terminal (HUD ANSI)
./build/src/app/DroneFlightSim --gui    # modo gráfico 3D (requiere raylib)
```

> El modo terminal interactivo está soportado en macOS/Linux; en Windows usa el modo gráfico.

## Controles

| Tecla | Acción |
|---|---|
| `W`/`A`/`S`/`D` o flechas | mover |
| `Espacio` | subir un metro; dos pulsaciones seguidas, subir sin parar; otra lo corta |
| — | para bajar no hay tecla: se suelta y baja por gravedad |
| `H` | mantener altitud |
| `F1`/`F2`, `F3`/`F4` | trim de cabeceo, de alabeo |
| `P` | pausa |
| `F5` / `F9` | guardar / cargar partida |
| `R` | reiniciar (tras fin de partida) |
| `X` o `Esc` | salir |

`DroneFlightSim --help` muestra esta ayuda; `--version`, la versión.

La partida se guarda en el directorio de datos del usuario (`~/Library/Application Support/Juego-Drone` en macOS, `$XDG_DATA_HOME/Juego-Drone` en Linux) y se carga automáticamente al arrancar. Los parámetros de juego se ajustan en [assets/config/game.toml](assets/config/game.toml) sin recompilar.

## Tests

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Suite: unitarios (Vec3, Drone, física, progresión, EventBus), integración (`World` con semilla determinista) y humo E2E del binario.

## Estructura

```
Juego-Drone/
├── src/
│   ├── core/        # drone_core: simulación pura, sin I/O
│   │   └── math/    # Vec3
│   ├── frontend/    # IRenderer / IInputSource + implementación terminal
│   └── app/         # main: composición e inyección
├── tests/           # Catch2: unit/ + integration/ + humo E2E
├── docs/            # arquitectura, estilo, ADRs, contribución
├── assets/          # configuración y niveles (Fase 2+)
└── PLAN3.md         # hoja de ruta vigente
```

La regla de arquitectura central: `src/core/` no contiene ninguna operación de I/O (la CI lo verifica con un guard automático) y no conoce las implementaciones concretas del frontend.

## Contribuir

Ver [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md), [docs/style.md](docs/style.md) y los ADRs en [docs/adr/](docs/adr/).

## Licencia

MIT. Ver [LICENSE](LICENSE).
