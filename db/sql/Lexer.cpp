#include "Lexer.hpp"

#include <cctype>
#include <iostream>
#include <string>

bool compare_case_insensitive(const std::string& lhs, const std::string& rhs) {
    for (auto l = lhs.begin(), r = rhs.begin(); l != lhs.end() && r != rhs.end(); l++, r++) {
        char c1 = (*l > 97) ? *l - 32 : *l;
        char c2 = (*r > 97) ? *r - 32 : *r;

        if (c1 != c2)
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
        if (next == EOF) {
            tokens.push_back(Token { .type = Token::Type::Eof, .value = "EOF" });
            return tokens;
        }

        if (isalpha(next)) {
            auto id = consume_identifier();
            if (compare_case_insensitive(id, "FROM")) {
                tokens.push_back(Token { .type = Token::Type::KeywordFrom, .value = "FROM" });
            }
            else if (compare_case_insensitive(id, "SELECT")) {
                tokens.push_back(Token { .type = Token::Type::KeywordSelect, .value = "SELECT" });
            }
            else if (compare_case_insensitive(id, "CREATE")) {
                tokens.push_back(Token { .type = Token::Type::KeywordCreate, .value = "CREATE" });
            }
            else if (compare_case_insensitive(id, "TABLE")) {
                tokens.push_back(Token { .type = Token::Type::KeywordTable, .value = "TABLE" });
            }
            else if (compare_case_insensitive(id, "AS")) {
                tokens.push_back(Token { .type = Token::Type::KeywordAlias, .value = "AS" });
            }
            else if (compare_case_insensitive(id, "TOP")) {
                tokens.push_back(Token { .type = Token::Type::KeywordTop, .value = "TOP" });
            }
            else if (compare_case_insensitive(id, "BY")) {
                tokens.push_back(Token { .type = Token::Type::KeywordBy, .value = "BY" });
            }
            else if (compare_case_insensitive(id, "ORDER")) {
                tokens.push_back(Token { .type = Token::Type::KeywordOrder, .value = "ORDER" });
            }
            else if (compare_case_insensitive(id, "WHERE")) {
                tokens.push_back(Token { .type = Token::Type::KeywordWhere, .value = "WHERE" });
            }
            else if (compare_case_insensitive(id, "BETWEEN")) {
                tokens.push_back(Token { .type = Token::Type::KeywordBetween, .value = "BETWEEN" });
            }
            else if (compare_case_insensitive(id, "AND")) {
                tokens.push_back(Token { .type = Token::Type::OpAnd, .value = "AND" });
            }
            else if (compare_case_insensitive(id, "OR")) {
                tokens.push_back(Token { .type = Token::Type::OpOr, .value = "OR" });
            }
            else if (compare_case_insensitive(id, "NOT")) {
                tokens.push_back(Token { .type = Token::Type::OpNot, .value = "NOT" });
            }
            else if (compare_case_insensitive(id, "LIKE")) {
                tokens.push_back(Token { .type = Token::Type::OpLike, .value = "LIKE" });
            }
            else if (compare_case_insensitive(id, "ASC") || compare_case_insensitive(id, "DESC")) {
                tokens.push_back(Token { .type = Token::Type::OrderByParam, .value = id });
            }
            else {
                tokens.push_back(Token { .type = Token::Type::Identifier, .value = id });
            }
        }
        else if (isspace(next)) {
            m_in >> std::ws;
        }
        else if (std::isdigit(next)) {
            auto number = consume_number();

            tokens.push_back(Token { .type = Token::Type::Number, .value = number });
        }
        else if (next == '*') {
            tokens.push_back(Token { .type = Token::Type::Asterisk, .value = "*" });
            m_in.get();
        }
        else if (next == ',') {
            tokens.push_back(Token { .type = Token::Type::Comma, .value = "," });
            m_in.get();
        }
        else if (next == '(') {
            tokens.push_back(Token { .type = Token::Type::ParenOpen, .value = "(" });
            m_in.get();
        }
        else if (next == ')') {
            tokens.push_back(Token { .type = Token::Type::ParenClose, .value = ")" });
            m_in.get();
        }
        else if (next == '[') {
            tokens.push_back(Token { .type = Token::Type::SquaredParenOpen, .value = "[" });
            m_in.get();
        }
        else if (next == ']') {
            tokens.push_back(Token { .type = Token::Type::SquaredParenClose, .value = "]" });
            m_in.get();
        }
        else if (next == ';') {
            tokens.push_back(Token { .type = Token::Type::Semicolon, .value = ";" });
            m_in.get();
        }
        else if (next == '=') {
            tokens.push_back(Token { .type = Token::Type::OpEqual, .value = "=" });
            m_in.get();
        }
        else if (next == '<') {
            tokens.push_back(Token { .type = Token::Type::OpLess, .value = "<" });
            m_in.get();
        }
        else if (next == '>') {
            tokens.push_back(Token { .type = Token::Type::OpGreater, .value = ">" });
            m_in.get();
        }
        else if (next == '!') {
            tokens.push_back(Token { .type = Token::Type::Exclamation, .value = "!" });
            m_in.get();
        }
        else if (next == '\'') {
            tokens.push_back(Token { .type = Token::Type::Quote, .value = "'" });
            m_in.get();
        }
        else {
            tokens.push_back(Token { .type = Token::Type::Garbage, .value = { next } });
            m_in.get();
        }
    }
}

}
