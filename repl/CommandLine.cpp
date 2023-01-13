#include "CommandLine.hpp"

#include <EssaUtil/Error.hpp>
#include <EssaUtil/ScopeGuard.hpp>
#include <EssaUtil/Stream.hpp>
#include <EssaUtil/Stream/Reader.hpp>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace CommandLine {

class GetLineSession {
public:
    explicit GetLineSession(Line const& line)
        : m_line(line) { }

    Util::ErrorOr<Util::UString, GetLineResult, Util::OsError> run();

private:
    void redraw();
    void bell() { std::cout << (char)0x07 << std::flush; }
    Util::Buffer m_input;
    Line const& m_line;
};

void GetLineSession::redraw() {
    std::cout << "\r\e[K\e[m"; // move cursor to beginning, clear line,
    std::cout << m_line.m_prompt.encode();
    std::cout.write(reinterpret_cast<char const*>(m_input.begin()), m_input.size());
    std::cout << std::flush;
}

Util::ErrorOr<Util::UString, GetLineResult, Util::OsError> GetLineSession::run() {
    // Standard input - disable echoing + line buffering
    termios old_termios;
    tcgetattr(STDIN_FILENO, &old_termios);
    ::termios new_termios = old_termios;
    new_termios.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    Util::ScopeGuard guard { [&] {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    } };

    redraw();

    Util::BinaryReader reader { Util::std_in() };

    while (true) {
        auto maybe_input = TRY(reader.get());
        if (!maybe_input) {
            return GetLineResult::EndOfFile;
        }
        auto input = *maybe_input;
        // std::cout << std::hex << (int)input << std::endl;
        if ((input >= 0x20 && input < 0x7f)) { // ASCII
            std::cout << input << std::flush;
            m_input.append(input);
            redraw();
        }
        else if (input == 0x04) { // EOF
            std::cout << "\n";
            return GetLineResult::EndOfFile;
        }
        else if (input == 0x0c) { // Form feed
            // Clear screen, move cursor to 1,1
            std::cout << "\e[2J\e[H" << std::flush;
            redraw();
        }
        else if (input == 0x7f) { // Backspace
            // FIXME: Implement proper UTF-8 support instead of just popping a single byte
            if (m_input.size() != 0) {
                m_input.take_from_back();
                redraw();
            }
            else {
                bell();
            }
        }
        else if (input == '\e') {
            // TODO
        }
        else if (input == '\n') {
            std::cout << std::endl;
            break;
        }
        else {
        }
    }

    return m_input.decode();
}

Util::ErrorOr<Util::UString, GetLineResult, Util::OsError> Line::get() {
    GetLineSession read { *this };
    return read.run();
}

}
