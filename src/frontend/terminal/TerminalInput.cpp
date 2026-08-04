#include "frontend/terminal/TerminalInput.h"

#if defined(_WIN32)
#include <conio.h>
#else
#include <poll.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#endif

namespace drone {

namespace {

Command mapKey(int key) {
    switch (key) {
        case 'W':
        case 'w':
            return Command::ThrustForward;
        case 'S':
        case 's':
            return Command::ThrustBackward;
        case 'A':
        case 'a':
            return Command::StrafeLeft;
        case 'D':
        case 'd':
            return Command::StrafeRight;
        case 'Q':
        case 'q':
            return Command::Ascend;
        case 'E':
        case 'e':
            return Command::Descend;
        case 'P':
        case 'p':
            return Command::Pause;
        case 'R':
        case 'r':
            return Command::Restart;
        case 'X':
        case 'x':
            return Command::Quit;
        case '1':
            return Command::Option1;
        case '2':
            return Command::Option2;
        case '3':
            return Command::Option3;
        default:
            return Command::None;
    }
}

}  // namespace

#if defined(_WIN32)

TerminalInput::TerminalInput() = default;
TerminalInput::~TerminalInput() = default;

Command TerminalInput::poll() {
    if (m_eof)
        return Command::Quit;
    if (_kbhit() == 0)
        return Command::None;
    const int key = _getch();
    if (key == 27)
        return Command::Quit;  // Esc
    return mapKey(key);
}

#else

TerminalInput* TerminalInput::s_instance = nullptr;

void TerminalInput::restoreTerminal() {
    if (m_rawMode) {
        tcsetattr(STDIN_FILENO, TCSANOW, &m_savedTermios);
        m_rawMode = false;
    }
}

void TerminalInput::onSignal(int sig) {
    if (s_instance)
        s_instance->restoreTerminal();
    // Re-raise con el handler por defecto para que el proceso termine.
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void TerminalInput::onExit() {
    if (s_instance)
        s_instance->restoreTerminal();
}

TerminalInput::TerminalInput() {
    if (isatty(STDIN_FILENO) == 1 && tcgetattr(STDIN_FILENO, &m_savedTermios) == 0) {
        termios raw = m_savedTermios;
        raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
            m_rawMode = true;
            s_instance = this;
            std::atexit(onExit);
            // ISIG sigue activo: Ctrl+C entrega SIGINT y sin handler la
            // terminal quedaría en raw. SIGTERM solo no basta (PLAN3 P2-6).
            std::signal(SIGTERM, onSignal);
            std::signal(SIGINT, onSignal);
            std::signal(SIGHUP, onSignal);
        }
    }
}

TerminalInput::~TerminalInput() {
    restoreTerminal();
    s_instance = nullptr;
}

int TerminalInput::readByte() {
    pollfd fd{STDIN_FILENO, POLLIN, 0};
    const int ready = ::poll(&fd, 1, 0);
    if (ready <= 0)
        return -1;
    unsigned char byte = 0;
    const ssize_t n = ::read(STDIN_FILENO, &byte, 1);
    if (n == 0)
        return -2;  // EOF: la fuente se agotó (cierra B3)
    if (n < 0)
        return -1;
    return byte;
}

Command TerminalInput::poll() {
    if (m_eof)
        return Command::Quit;

    const int byte = readByte();
    if (byte == -1)
        return Command::None;
    if (byte == -2) {
        m_eof = true;
        return Command::Quit;
    }

    if (byte == 27) {  // Esc: ¿tecla suelta o secuencia?
        const int second = readByte();
        if (second != '[')
            return Command::Quit;
        const int third = readByte();
        switch (third) {
            case 'A':
                return Command::Ascend;  // ↑
            case 'B':
                return Command::Descend;  // ↓
            case 'C':
                return Command::StrafeRight;  // →
            case 'D':
                return Command::StrafeLeft;  // ←
            case '1': {                      // F5: ESC [ 1 5 ~
                const int fourth = readByte();
                if (fourth == '5' && readByte() == '~')
                    return Command::Save;
                return Command::None;
            }
            case '2': {  // F9: ESC [ 2 0 ~
                const int fourth = readByte();
                if (fourth == '0' && readByte() == '~')
                    return Command::Load;
                return Command::None;
            }
            default:
                return Command::None;
        }
    }

    return mapKey(byte);
}

#endif

}  // namespace drone
