#pragma once

#include <string>
#include <vector>

namespace Db::Sql {

struct Token {
    enum class Type {
        KeywordSelect,
        KeywordFrom,
        Identifier,
        Asterisk,
        Garbage
    };

    Type type {};
    std::string value {};
};

class Lexer {
public:
    Lexer(std::istream& in)
        : m_in(in) { }

    std::vector<Token> lex();

private:
    std::istream& m_in;
};

}
