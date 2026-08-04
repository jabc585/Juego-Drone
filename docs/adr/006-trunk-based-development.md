# ADR-006: Trunk-based development

**Estado:** Aceptado
**Fecha:** 2026-08-04

## Contexto

Proyecto con un solo desarrollador. Se necesita una estrategia de branching que maximice velocidad sin sacrificar calidad.

## Decision

Trunk-based development: ramas cortas (`feature/<nombre>`, `fix/<nombre>`) que se mergean a `main` en < 48h. `main` siempre compila y pasa tests.

## Alternativas consideradas

- **GitFlow:** Demasiada ceremonia para un solo desarrollador (develop, release, hotfix). Ralentiza sin beneficio.
- **GitHub Flow:** Similar a trunk-based pero con deploy desde feature branches. Sobredimensionado (no hay entorno de staging).

## Consecuencias

- PRs pequeños y frecuentes
- CI en cada push a `main` y PR
- Commits convencionales como unica formalidad
- Sin ramas de larga duracion que acumulen conflictos
