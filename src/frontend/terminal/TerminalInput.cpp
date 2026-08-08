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
        case ' ':
            return Command::Ascend;
        case 'P':
        case 'p':
            return Command::Pause;
        case 'H':
        case 'h':
            return Command::AltitudeToggle;
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
            return Command::Unknown;
    }
}

// F1-F4 ajustan trims, F5/F9 guardan y cargan. Los numeros son los codigos
// CSI de xterm (F1=11 ... F5=15, F9=20).
Command mapFunctionKey(int code) {
    switch (code) {
        case 11:
            return Command::TrimPitchUp;
        case 12:
            return Command::TrimPitchDown;
        case 13:
            return Command::TrimRollLeft;
        case 14:
            return Command::TrimRollRight;
        case 15:
            return Command::Save;
        case 20:
            return Command::Load;
        default:
            return Command::Unknown;
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

int TerminalInput::readByte(int timeoutMs) {
    pollfd fd{STDIN_FILENO, POLLIN, 0};
    const int ready = ::poll(&fd, 1, timeoutMs);
    if (ready <= 0)
        return kNoByte;
    unsigned char byte = 0;
    const ssize_t n = ::read(STDIN_FILENO, &byte, 1);
    if (n == 0)
        return kEof;  // EOF: la fuente se agotó (cierra B3)
    if (n < 0)
        return kNoByte;
    return byte;
}

// Los bytes que siguen a Esc pueden llegar en lecturas distintas (ssh, tmux,
// terminales lentas). Sin margen de espera una flecha se leia como Esc suelto
// y cerraba la partida, asi que aqui si esperamos.
int TerminalInput::readSequenceByte() {
    const int byte = readByte(kEscapeTimeoutMs);
    if (byte == kEof)
        m_eof = true;
    return byte;
}

Command TerminalInput::parseEscape() {
    const int second = readSequenceByte();
    if (second == kEof)
        return Command::Quit;
    if (second == kNoByte)
        return Command::Quit;  // Esc suelto: nadie continua la secuencia

    // SS3: como manda Terminal.app, iTerm2 y xterm las F1-F4, y las flechas
    // cuando el terminal esta en modo aplicacion.
    if (second == 'O') {
        switch (readSequenceByte()) {
            case 'P':
                return Command::TrimPitchUp;  // F1
            case 'Q':
                return Command::TrimPitchDown;  // F2
            case 'R':
                return Command::TrimRollLeft;  // F3
            case 'S':
                return Command::TrimRollRight;  // F4
            case 'A':
                return Command::ThrustForward;
            case 'B':
                return Command::ThrustBackward;
            case 'C':
                return Command::StrafeRight;
            case 'D':
                return Command::StrafeLeft;
            case kEof:
                return Command::Quit;
            default:
                return Command::Unknown;
        }
    }
    if (second != '[')
        return Command::Unknown;  // secuencia que no entendemos: se ignora

    const int third = readSequenceByte();
    switch (third) {
        case 'A':
            return Command::ThrustForward;  // ↑
        case 'B':
            return Command::ThrustBackward;  // ↓
        case 'C':
            return Command::StrafeRight;  // →
        case 'D':
            return Command::StrafeLeft;  // ←
        case kEof:
            return Command::Quit;
        default:
            break;
    }
    if (third < '0' || third > '9')
        return Command::Unknown;

    // CSI numerico: ESC [ <codigo> ~, con posible modificador ESC [ <codigo> ; <mod> ~
    int code = third - '0';
    for (int i = 0; i < 4; ++i) {
        const int next = readSequenceByte();
        if (next == '~')
            return mapFunctionKey(code);
        if (next == kEof)
            return Command::Quit;
        if (next < '0' || next > '9')
            return Command::Unknown;
        code = code * 10 + (next - '0');
    }
    return Command::Unknown;
}

Command TerminalInput::poll() {
    if (m_eof)
        return Command::Quit;

    const int byte = readByte(0);
    if (byte == kNoByte)
        return Command::None;
    if (byte == kEof) {
        m_eof = true;
        return Command::Quit;
    }
    if (byte == 27)
        return parseEscape();

    return mapKey(byte);
}

#endif

}  // namespace drone
