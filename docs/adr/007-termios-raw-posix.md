# ADR-007: Entrada raw de terminal POSIX sin ncurses

**Estado:** Aceptado
**Fecha:** 2026-08-04

## Contexto

El juego necesita entrada de teclado no bloqueante en terminal. El bucle original usaba `std::cin >>` (bloqueante). Se necesitan leer teclas individuales sin esperar Enter.

## Decision

Usar `termios` en modo raw (no canonico, sin eco) + `poll(0)` para lectura no bloqueante en POSIX. Mantener `ISIG` para que Ctrl+C siga funcionando. Windows usa `_kbhit`/`_getch` de `<conio.h>` como fallback minimo (solo compila en CI, la experiencia interactiva en Windows llega con raylib en Fase 3).

## Alternativas consideradas

- **ncurses:** Biblioteca completa de UI de terminal. Excesiva para leer teclas (~40 lineas de codigo). Introduce dependencia.
- **PDCurses (Windows):** Port de ncurses. Mismo problema de sobredimension.
- **readline:** Orientado a entrada por linea, no a teclas individuales.
- **termios raw:** ~40 lineas aisladas en TerminalInput. Sin dependencias nuevas. ISIG conservado para Ctrl+C.

## Consecuencias

- Sin dependencias nuevas para entrada de terminal
- Codigo aislado en `TerminalInput` (frontend), el core no lo ve
- Windows queda sin soporte interactivo de terminal (aceptable: CI compila, experiencia real llega con raylib)
- F5/F9 y secuencias de escape manejadas con parser de bytes, no con biblioteca externa
