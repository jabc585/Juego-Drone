# ADR-002: raylib vs SDL2 para el frontend gráfico

**Estado:** Aceptado
**Fecha:** 2026-08-04

## Contexto

La Fase 3 requiere un frontend gráfico. Los comentarios del código original sugieren SDL2. Se evalúa la alternativa raylib.

## Decisión

Usar **raylib** como biblioteca gráfica.

## Alternativas consideradas

- **SDL2:** API en C, requiere extensiones para gráficos (SDL_image, SDL_ttf), más verbosa. Madura y ubicua pero más código para resultados equivalentes.
- **SFML:** API en C++, solo 2D nativamente. No considerada.
- **raylib:** API en C, todo-en-uno (gráficos 2D/3D, entrada, audio), ~20 funciones para un loop de juego básico. Ideal para el alcance de este proyecto (~2 MB compilado).

## Consecuencias

- Menos dependencias (raylib reemplaza SDL2 + OpenGL)
- API más simple → iteración más rápida en Fase 3
- La física y lógica del core no cambian (la separación core/frontend lo garantiza)
