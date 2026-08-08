# ADR-004: Fisica propia vs Bullet Physics

**Estado:** Reemplazado por [ADR-008](008-rp3d-sustituye-fisica-propia.md)
**Fecha:** 2026-08-04

## Contexto

El codigo original mencionaba Bullet Physics para calculos avanzados. Se evalua si integrar un motor de fisica externo o mantener una implementacion propia.

## Decision

Implementar fisica propia con AABB simple para colisiones. No usar Bullet Physics.

## Alternativas consideradas

- **Bullet Physics:** Motor de cuerpos rigidos completo, soporte 3D, deteccion de colisiones avanzada. Sobredimensionado para un dron con < 10 obstaculos.
- **Box2D:** Solo 2D, no aplica.
- **Fisica propia:** ~200 lineas, Euler semi-implicito, AABB por minima penetracion. Suficiente para el alcance actual.

## Consecuencias

- Sin dependencia pesada (~2 MB Bullet) para una necesidad de ~200 LOC
- Comportamiento totalmente predecible y testeable
- Si el proyecto escala a mundos complejos, migrar a Bullet es posible (PhysicsEngine encapsula toda la fisica)
