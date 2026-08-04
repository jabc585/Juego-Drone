# ADR-003: Catch2 vs GoogleTest para testing

**Estado:** Aceptado
**Fecha:** 2026-08-04

## Contexto

Se necesita un framework de testing C++ integrado con CTest. Las opciones principales son Catch2 y GoogleTest.

## Decisión

Usar **Catch2 v3**.

## Alternativas consideradas

- **GoogleTest:** Más ubicuo en la industria, buena integración con CMake. Sintaxis más verbosa (macros EXPECT/ASSERT separados), requiere instalación o build separado.
- **Catch2 v3:** Header-only opcional, sintaxis BDD (`REQUIRE`, `SECTION`, `GIVEN/WHEN/THEN`), integración nativa con CTest vía `catch_discover_tests`. Más ligero para proyectos pequeños.

## Consecuencias

- Menos boilerplate en tests
- `catch_discover_tests` registra tests automáticamente en CTest
- Integración vía `FetchContent` con tag fijo `v3.5.0`
