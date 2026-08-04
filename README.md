# Juego-Drone

[![CI](https://github.com/jabc585/Juego-Drone/actions/workflows/ci.yml/badge.svg)](https://github.com/jabc585/Juego-Drone/actions/workflows/ci.yml)

Simulador de vuelo de dron en terminal, en tiempo real: física con empuje, gravedad, arrastre y rachas de viento; batería; obstáculos; progresión por niveles y HUD ANSI a 60 FPS.

## Requisitos

- CMake ≥ 3.21
- Compilador C++17 (Clang, GCC o MSVC)

## Compilación y ejecución

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/src/app/DroneFlightSim
```

> El frontend interactivo de terminal está soportado en macOS/Linux; en Windows el soporte interactivo llega con el frontend gráfico (ver [PLAN3.md](PLAN3.md)).

## Controles

| Tecla | Acción |
|---|---|
| `W`/`A`/`S`/`D` o flechas | mover |
| `Q` / `E` | ascender / descender |
| `P` | pausa |
| `R` | reiniciar (tras fin de partida) |
| `X` o `Esc` | salir |

`DroneFlightSim --help` muestra esta ayuda; `--version`, la versión.

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
