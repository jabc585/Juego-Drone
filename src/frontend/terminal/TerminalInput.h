#pragma once

#include "frontend/IInputSource.h"

#if !defined(_WIN32)
#include <termios.h>
#endif

namespace drone {

class TerminalInput : public IInputSource {
public:
    TerminalInput();
    ~TerminalInput() override;

    TerminalInput(const TerminalInput&) = delete;
    TerminalInput& operator=(const TerminalInput&) = delete;

    Command poll() override;

private:
#if !defined(_WIN32)
    static constexpr int kNoByte = -1;
    static constexpr int kEof = -2;
    // Margen para que lleguen los bytes que siguen a Esc. Solo se paga al
    // pulsar Esc o una tecla de secuencia, nunca en el camino normal.
    static constexpr int kEscapeTimeoutMs = 100;

    int readByte(int timeoutMs);
    int readSequenceByte();
    Command parseEscape();

    termios m_savedTermios{};
    bool m_rawMode = false;

    // PLAN3 P2-6: restauracion de terminal ante muerte anomala
    static TerminalInput* s_instance;
    void restoreTerminal();
    static void onSignal(int sig);
    static void onExit();
#endif
    bool m_eof = false;
};

}  // namespace drone
