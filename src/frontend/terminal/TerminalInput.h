#pragma once

#include "frontend/IInputSource.h"

#if !defined(_WIN32)
#include <termios.h>
#endif

namespace drone {

// Entrada de teclado no bloqueante (ADR-007 / PLAN2.md R5).
// POSIX: modo raw con termios + poll(). Windows: _kbhit/_getch de <conio.h>
// como soporte mínimo para que la CI compile; el frontend interactivo oficial
// en Windows llega con raylib en Fase 3.
class TerminalInput : public IInputSource {
public:
    TerminalInput();
    ~TerminalInput() override;

    TerminalInput(const TerminalInput&) = delete;
    TerminalInput& operator=(const TerminalInput&) = delete;

    Command poll() override;

private:
#if !defined(_WIN32)
    int readByte();  // -1 sin datos, -2 EOF, si no el byte leído

    termios m_savedTermios{};
    bool m_rawMode = false;
#endif
    bool m_eof = false;
};

}  // namespace drone
