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
    int readByte();

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
