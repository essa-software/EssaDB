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
    GetLineErrorOr<uint8_t> get_byte();
    GetLineErrorOr<void> handle_csi();
    std::string get_input_text() const;
    void select_current_history_entry();
    void insert(Util::UString str);

    Util::BinaryReader m_reader { Util::std_in() };
    Util::UString m_input;
    // Temporary buffer used for building codepoinds from UTF-8 characters.
    Util::Buffer m_codepoint_buffer;
    Line const& m_line;
    size_t m_cursor = 0;
    size_t m_history_index = 0;
};

void GetLineSession::redraw() {
    std::cout << "\r\e[K\e[m"; // move cursor to beginning, clear line, clear style
    std::cout << m_line.m_prompt.encode();
    std::cout << get_input_text();
    std::cout << "\e[" << (m_cursor + 3) << "G";
    std::cout << std::flush;
}

GetLineErrorOr<uint8_t> GetLineSession::get_byte() {
    auto input = TRY(m_reader.get());
    if (!input) {
        return GetLineResult::EndOfFile;
    }
    return *input;
}

GetLineErrorOr<void> GetLineSession::handle_csi() {
    // From Wikipedia (https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_(Control_Sequence_Introducer)_sequences):
    // The 'ESC [' is followed by any number (including none) of
    // "parameter bytes" in the range 0x30–0x3F (ASCII 0–9:;<=>?),
    auto parameters = TRY(m_reader.read_while([&](uint8_t byte) {
        return byte >= 0x30 && byte <= 0x3f;
    }));

    // then by any number of "intermediate bytes"
    // in the range 0x20–0x2F (ASCII space and !"#$%&'()*+,-./),
    auto intermediate = TRY(m_reader.read_while([&](uint8_t byte) {
        return byte >= 0x20 && byte <= 0x2f;
    }));

    // then finally by a single "final byte" in the range 0x40–0x7E (ASCII @A–Z[\]^_`a–z{|}~).
    auto final_byte = TRY(get_byte());
    if (!(final_byte >= 0x40 && final_byte <= 0x7e)) {
        return {};
    }

    switch (final_byte) {
    case 'A': // up
        if (m_input.size() == 0) {
            if (m_history_index < m_line.m_history.size()) {
                m_history_index++;
                m_cursor = get_input_text().size();
                redraw();
            }
        }
        break;
    case 'B': // down
        if (m_input.size() == 0) {
            if (m_history_index > 0) {
                m_history_index--;
                m_cursor = get_input_text().size();
                redraw();
            }
        }
        break;
    case 'C': // right
        if (m_cursor < get_input_text().size()) {
            m_cursor++;
            redraw();
        }
        break;
    case 'D': // left
        if (m_cursor > 0) {
            m_cursor--;
            redraw();
        }
        break;
    default:
        break;
    }
    return {};
}

std::string GetLineSession::get_input_text() const {
    if (m_history_index == 0) {
        return m_input.encode();
    }
    else {
        return m_line.m_history[m_line.m_history.size() - m_history_index].encode();
    }
}

void GetLineSession::select_current_history_entry() {
    if (m_input.size() == 0 && m_history_index != 0) {
        m_input = m_line.m_history[m_line.m_history.size() - m_history_index];
        m_history_index = 0;
    }
}

void GetLineSession::insert(Util::UString str) {
    select_current_history_entry();
    m_input = m_input.insert(std::move(str), m_cursor);
    m_cursor++;
    redraw();
}

GetLineErrorOr<Util::UString> GetLineSession::run() {
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

    while (true) {
        auto input = TRY(get_byte());
        // std::cout << std::hex << (int)input << std::endl;
        if (input & 0x80) { // UTF-8
            m_codepoint_buffer.append(input);
            auto decoded = m_codepoint_buffer.decode();
            if (!decoded.is_error()) {
                insert(decoded.release_value());
                m_codepoint_buffer.clear();
            }
        }
        else if (input >= 0x20 && input < 0x7f) { // ASCII
            insert(Util::UString { input });
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
            select_current_history_entry();
            if (m_cursor > 0) {
                m_input = m_input.erase(m_cursor - 1, 1);
                m_cursor--;
                redraw();
            }
            else {
                bell();
            }
        }
        else if (input == '\e') {
            if (TRY(get_byte()) == '[') {
                TRY(handle_csi());
            }
        }
        else if (input == '\n') {
            std::cout << std::endl;
            break;
        }
        else {
        }
    }

    return Util::UString { get_input_text() };
}

GetLineErrorOr<Util::UString> Line::get() {
    GetLineSession read { *this };
    auto result = TRY(read.run());
    if (!result.is_empty()) {
        if (!(!m_history.empty() && m_history.back() == result)) {
            m_history.push_back(result);
        }
    }
    return result;
}

}
