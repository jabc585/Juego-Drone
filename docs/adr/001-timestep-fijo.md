# ADR-001: Timestep fijo para el bucle de juego

**Estado:** Aceptado
**Fecha:** 2026-08-04

## Contexto

El bucle actual usa `deltaTime` variable medido con `high_resolution_clock`, pero la entrada bloqueante (`std::cin`) contamina la medición con el tiempo de espera del usuario. Se necesita un bucle determinista y testeable.

## Decisión

Implementar el patrón "Fix Your Timestep" (Gaffer on Games):
- Timestep fijo de 1/60 s (16.67 ms)
- Acumulador con clamp a 250 ms para evitar espiral de la muerte
- Entrada no bloqueante mediante `IInputSource::poll()`
- Render interpolado independiente de la física

## Alternativas consideradas

- **DeltaTime variable:** No determinista, difícil de testear.
- **Game loop con sleep:** Inexacto, dependiente del scheduler del SO.

## Consecuencias

- Física determinista y reproducible en tests
- Frame rate de simulación constante
- Requiere entrada no bloqueante (POSIX termios en Fase 2, raylib en Fase 3)
