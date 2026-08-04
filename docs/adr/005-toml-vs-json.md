# ADR-005: toml++ vs nlohmann/json para configuracion

**Estado:** Aceptado
**Fecha:** 2026-08-04

## Contexto

PLAN3 P0-2 requiere configuracion externa para GameConfig. Se necesita elegir formato y parser.

## Decision

Usar TOML con toml++ (header-only). Formato: `assets/config/game.toml`.

## Alternativas consideradas

- **JSON + nlohmann/json:** Ubicuo, buena biblioteca. JSON es verboso para archivos de configuracion escritos por humanos (sin comentarios, requiere comillas en claves).
- **YAML + yaml-cpp:** Bueno para humanos pero parser complejo, dependencia pesada.
- **TOML + toml++:** Diseñado especificamente para archivos de configuracion. Soporta comentarios, sintaxis limpia, header-only (~300 KB). Ideal para este alcance.

## Consecuencias

- Archivos de configuracion legibles y comentables
- toml++ es header-only, sin build separado
- FetchContent con tag fijo v3.4.0
- Para guardado de partida se usa el mismo formato TOML (consistencia)
