#pragma once

#include <EssaUtil/Stream.hpp>
#include <EssaUtil/UString.hpp>

namespace CommandLine {

enum class GetLineResult {
    EndOfFile, // ^D
};

using GetLineResultOrString = Util::ErrorOr<Util::UString, GetLineResult, Util::OsError>;

class Line {
public:
    void set_prompt(Util::UString prompt) { m_prompt = std::move(prompt); }

    Util::ErrorOr<Util::UString, GetLineResult, Util::OsError> get();

private:
    friend class GetLineSession;

    void redraw();

    Util::UString m_prompt;
};

}
