# Guía de Estilo C++ — Juego-Drone

## Formato

Se usa `clang-format` (configuración en `.clang-format`, base Google).

- 4 espacios de indentación (no tabs)
- 100 columnas máximo
- Llaves en la misma línea (`Attach`)

## Nomenclatura

| Elemento | Convención | Ejemplo |
|----------|-----------|---------|
| Clases / Structs | `PascalCase` | `PhysicsEngine`, `TerminalRenderer` |
| Métodos / Funciones | `camelCase` | `updatePhysics()`, `getPosition()` |
| Variables miembro | `m_` prefijo | `m_position`, `m_isRunning` |
| Constantes globales | `kPascalCase` | `kGravity`, `kFixedTimestep` |
| Constantes de clase | `kPascalCase` | `kMaxBattery` |
| Namespaces | `snake_case` | `drone::core`, `drone::math` |
| Archivos | `PascalCase.{h,cpp}` | `Drone.cpp`, `GameController.h` |

## Reglas

- **`const` correctness:** getters son `const`; parámetros de solo lectura son `const&`.
- **`[[nodiscard]]`** en getters y funciones cuyo retorno no deba ignorarse.
- **`explicit`** en constructores de un parámetro.
- **`#pragma once`** en todos los headers.
- **Prohibido `using namespace std;`** en headers.
- **Orden de includes:** propio → proyecto → bibliotecas → estándar (separados por línea en blanco).

## Gestión de errores

- El core nunca imprime ni termina el proceso.
- Excepciones solo para errores irrecuperables de inicialización.
- `assert()` en debug para invariantes.
