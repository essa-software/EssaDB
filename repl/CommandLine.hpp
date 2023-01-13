#pragma once

#include <EssaUtil/Stream.hpp>
#include <EssaUtil/UString.hpp>
#include <list>

namespace CommandLine {

enum class GetLineResult {
    EndOfFile, // ^D
};

template<class T>
using GetLineErrorOr = Util::ErrorOr<T, GetLineResult, Util::OsError>;

class Line {
public:
    void set_prompt(Util::UString prompt) { m_prompt = std::move(prompt); }

    GetLineErrorOr<Util::UString> get();

private:
    friend class GetLineSession;

    void redraw();

    Util::UString m_prompt;
    std::vector<Util::UString> m_history;
};

}
