#include <unistd.h>

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <string>
#include <thread>

#include "core/Commands.h"
#include "frontend/terminal/TerminalInput.h"

using drone::Command;
using drone::TerminalInput;

namespace {

// Sustituye stdin por una tuberia para inyectar bytes crudos. Sin tty no se
// activa el modo raw, que es justo lo que interesa: solo se ejerce el parser.
class StdinPipe {
public:
    StdinPipe() {
        m_saved = ::dup(STDIN_FILENO);
        int fds[2] = {-1, -1};
        REQUIRE(::pipe(fds) == 0);
        m_read = fds[0];
        m_write = fds[1];
        REQUIRE(::dup2(m_read, STDIN_FILENO) != -1);
    }

    ~StdinPipe() {
        ::dup2(m_saved, STDIN_FILENO);
        ::close(m_saved);
        closeWriter();
        ::close(m_read);
    }

    StdinPipe(const StdinPipe&) = delete;
    StdinPipe& operator=(const StdinPipe&) = delete;

    void send(const std::string& bytes) const {
        REQUIRE(::write(m_write, bytes.data(), bytes.size()) == static_cast<ssize_t>(bytes.size()));
    }

    void closeWriter() {
        if (m_write != -1) {
            ::close(m_write);
            m_write = -1;
        }
    }

private:
    int m_saved = -1;
    int m_read = -1;
    int m_write = -1;
};

}  // namespace

TEST_CASE("TerminalInput: las flechas mueven, no cierran la partida") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.send("\x1b[A");
    REQUIRE(input.poll() == Command::ThrustForward);
    pipe.send("\x1b[B");
    REQUIRE(input.poll() == Command::ThrustBackward);
    pipe.send("\x1b[C");
    REQUIRE(input.poll() == Command::StrafeRight);
    pipe.send("\x1b[D");
    REQUIRE(input.poll() == Command::StrafeLeft);
}

// Terminal.app, iTerm2 y xterm mandan F1-F4 en SS3, no en CSI. Antes esto
// caia en la rama "no empieza por [" y cerraba el juego.
TEST_CASE("TerminalInput: F1-F4 en SS3 ajustan los trims") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.send("\x1bOP");
    REQUIRE(input.poll() == Command::TrimPitchUp);
    pipe.send("\x1bOQ");
    REQUIRE(input.poll() == Command::TrimPitchDown);
    pipe.send("\x1bOR");
    REQUIRE(input.poll() == Command::TrimRollLeft);
    pipe.send("\x1bOS");
    REQUIRE(input.poll() == Command::TrimRollRight);
}

TEST_CASE("TerminalInput: F1-F5 y F9 en CSI") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.send("\x1b[11~");
    REQUIRE(input.poll() == Command::TrimPitchUp);
    pipe.send("\x1b[14~");
    REQUIRE(input.poll() == Command::TrimRollRight);
    pipe.send("\x1b[15~");
    REQUIRE(input.poll() == Command::Save);
    pipe.send("\x1b[20~");
    REQUIRE(input.poll() == Command::Load);
}

// El caso que cerraba partidas por ssh o tmux: la secuencia llega troceada
// entre lecturas y el parser tiene que esperar a que se complete.
TEST_CASE("TerminalInput: una secuencia troceada sigue siendo una tecla") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.send("\x1b");
    std::thread resto([&pipe] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        pipe.send("[");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        pipe.send("A");
    });
    const Command cmd = input.poll();
    resto.join();
    REQUIRE(cmd == Command::ThrustForward);
}

// La elevacion es de la barra espaciadora; Q y E ya no hacen nada.
TEST_CASE("TerminalInput: el espacio eleva y Q/E quedan libres") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.send(" ");
    REQUIRE(input.poll() == Command::Ascend);
    pipe.send("q");
    REQUIRE(input.poll() == Command::Unknown);
    pipe.send("e");
    REQUIRE(input.poll() == Command::Unknown);
}

TEST_CASE("TerminalInput: Esc suelto sigue saliendo") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.send("\x1b");
    REQUIRE(input.poll() == Command::Quit);
}

// Configuracion promete "tecla para volver": una tecla sin accion tiene que
// distinguirse de "no se ha pulsado nada".
TEST_CASE("TerminalInput: una tecla sin accion no es None") {
    StdinPipe pipe;
    TerminalInput input;

    REQUIRE(input.poll() == Command::None);
    pipe.send("z");
    REQUIRE(input.poll() == Command::Unknown);
    pipe.send("w");
    REQUIRE(input.poll() == Command::ThrustForward);
}

TEST_CASE("TerminalInput: EOF termina la partida (B3)") {
    StdinPipe pipe;
    TerminalInput input;

    pipe.closeWriter();
    REQUIRE(input.poll() == Command::Quit);
    REQUIRE(input.poll() == Command::Quit);  // se mantiene, no vuelve a leer
}
