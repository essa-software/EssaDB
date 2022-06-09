#include "Lexer.hpp"

#include <cctype>
#include <iostream>
#include <string>

bool compare_case_insensitive(const std::string& lhs, const std::string& rhs){
    for(auto l = lhs.begin(), r = rhs.begin(); l != lhs.end() && r != rhs.end(); l++, r++){
        char c1 = (*l > 97) ? *l - 32 : *l;
        char c2 = (*r > 97) ? *r - 32 : *r;

        if(c1 != c2)
            return false;
    }

    return true;
}

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

    auto consume_number = [&]() {
        std::string s;
        while (isdigit(m_in.peek())) {
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
            if (compare_case_insensitive(id, "FROM")) {
                tokens.push_back(Token { .type = Token::Type::KeywordFrom, .value = "FROM" });
            }
            else if (compare_case_insensitive(id, "SELECT")) {
                tokens.push_back(Token { .type = Token::Type::KeywordSelect, .value = "SELECT" });
                m_select_syntax = 1;
            }
            else if (m_select_syntax && (compare_case_insensitive(id, "TOP") || compare_case_insensitive(id, "DISTINCT"))) {
                tokens.push_back(Token { .type = Token::Type::KeywordAfterSelect, .value = id });
            }
            else {
                tokens.push_back(Token { .type = Token::Type::Identifier, .value = id });
                m_select_syntax = 0;
            }
        }
        else if (isspace(next)) {
            m_in >> std::ws;
        }
        else if (std::isdigit(next)) {
            auto number = consume_number();

            tokens.push_back(Token { .type = Token::Type::Arg, .value = number });
        }
        else if (next == '*') {
            tokens.push_back(Token { .type = Token::Type::Asterisk, .value = "*" });
            m_in.get();
        }
        else if (next == ',') {
            tokens.push_back(Token { .type = Token::Type::Comma, .value = "," });
            m_in.get();
        }
        else {
            tokens.push_back(Token { .type = Token::Type::Garbage, .value = { next } });
            m_in.get();
        }
    }
}

}
