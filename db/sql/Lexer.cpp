#include "Lexer.hpp"

#include <iostream>

namespace Db::Sql {

std::vector<Token> Lexer::lex() {
    std::vector<Token> tokens;

    auto consume_identifier = [&]() {
        std::string s;
        while (isalpha(m_in.peek())) {
            s += m_in.get();
        }
        return s;
    };

    while (true) {
        char next = m_in.peek();
        if (next == EOF)
            return tokens;

        if (isalpha(next)) {
            auto id = consume_identifier();
            // TODO: Case-insensitive match
            if (id == "FROM") {
                tokens.push_back(Token { .type = Token::Type::KeywordFrom, .value = "FROM" });
            }
            else if (id == "SELECT") {
                tokens.push_back(Token { .type = Token::Type::KeywordSelect, .value = "SELECT" });
            }
            else {
                tokens.push_back(Token { .type = Token::Type::Identifier, .value = id });
            }
        }
        else if (isspace(next)) {
            m_in >> std::ws;
        }
        else if (next == '*') {
            tokens.push_back(Token { .type = Token::Type::Asterisk, .value = "*" });
            m_in.get();
        }
        else {
            tokens.push_back(Token { .type = Token::Type::Garbage, .value = { next } });
            m_in.get();
        }
    }
}

}
