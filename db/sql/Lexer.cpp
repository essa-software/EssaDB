#include "Lexer.hpp"

#include <cctype>
#include <iostream>
#include <string>

bool compare_case_insensitive(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size())
        return false;
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
            m_in.clear();
            tokens.push_back(Token { .type = Token::Type::Eof, .value = "EOF", .start = m_in.tellg() });
            return tokens;
        }

        auto start = m_in.tellg();
        if (isalpha(next)) {
            auto id = consume_identifier();
            if (compare_case_insensitive(id, "FROM")) {
                tokens.push_back(Token { .type = Token::Type::KeywordFrom, .value = "FROM", .start = start });
            }
            else if (compare_case_insensitive(id, "SELECT")) {
                tokens.push_back(Token { .type = Token::Type::KeywordSelect, .value = "SELECT", .start = start });
            }
            else if (compare_case_insensitive(id, "CREATE")) {
                tokens.push_back(Token { .type = Token::Type::KeywordCreate, .value = "CREATE", .start = start });
            }
            else if (compare_case_insensitive(id, "TABLE")) {
                tokens.push_back(Token { .type = Token::Type::KeywordTable, .value = "TABLE", .start = start });
            }
            else if (compare_case_insensitive(id, "AS")) {
                tokens.push_back(Token { .type = Token::Type::KeywordAlias, .value = "AS", .start = start });
            }
            else if (compare_case_insensitive(id, "TOP")) {
                tokens.push_back(Token { .type = Token::Type::KeywordTop, .value = "TOP", .start = start });
            }
            else if (compare_case_insensitive(id, "BY")) {
                tokens.push_back(Token { .type = Token::Type::KeywordBy, .value = "BY", .start = start });
            }
            else if (compare_case_insensitive(id, "ORDER")) {
                tokens.push_back(Token { .type = Token::Type::KeywordOrder, .value = "ORDER", .start = start });
            }
            else if (compare_case_insensitive(id, "WHERE")) {
                tokens.push_back(Token { .type = Token::Type::KeywordWhere, .value = "WHERE", .start = start });
            }
            else if (compare_case_insensitive(id, "BETWEEN")) {
                tokens.push_back(Token { .type = Token::Type::KeywordBetween, .value = "BETWEEN", .start = start });
            }
            else if (compare_case_insensitive(id, "AND")) {
                tokens.push_back(Token { .type = Token::Type::OpAnd, .value = "AND", .start = start });
            }
            else if (compare_case_insensitive(id, "OR")) {
                tokens.push_back(Token { .type = Token::Type::OpOr, .value = "OR", .start = start });
            }
            else if (compare_case_insensitive(id, "NOT")) {
                tokens.push_back(Token { .type = Token::Type::OpNot, .value = "NOT", .start = start });
            }
            else if (compare_case_insensitive(id, "LIKE")) {
                tokens.push_back(Token { .type = Token::Type::OpLike, .value = "LIKE", .start = start });
            }
            else if (compare_case_insensitive(id, "ASC") || compare_case_insensitive(id, "DESC")) {
                tokens.push_back(Token { .type = Token::Type::OrderByParam, .value = id, .start = start });
            }
            else {
                tokens.push_back(Token { .type = Token::Type::Identifier, .value = id, .start = start });
            }
        }
        else if (isspace(next)) {
            m_in >> std::ws;
        }
        else if (std::isdigit(next)) {
            auto number = consume_number();

            tokens.push_back(Token { .type = Token::Type::Number, .value = number, .start = start });
        }
        else if (next == '*') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::Asterisk, .value = "*", .start = start });
        }
        else if (next == ',') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::Comma, .value = ",", .start = start });
        }
        else if (next == '(') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::ParenOpen, .value = "(", .start = start });
        }
        else if (next == ')') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::ParenClose, .value = ")", .start = start });
        }
        else if (next == '[') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::SquaredParenOpen, .value = "[", .start = start });
        }
        else if (next == ']') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::SquaredParenClose, .value = "]", .start = start });
        }
        else if (next == ';') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::Semicolon, .value = ";", .start = start });
        }
        else if (next == '=') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpEqual, .value = "=", .start = start });
        }
        else if (next == '<') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpLess, .value = "<", .start = start });
        }
        else if (next == '>') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpGreater, .value = ">", .start = start });
        }
        else if (next == '!') {
            m_in.get();
            next = m_in.peek();
            if (next == '=') {
                tokens.push_back(Token { .type = Token::Type::OpNotEqual, .value = "!=", .start = start });
                m_in.get();
            }
            else {
                tokens.push_back(Token { .type = Token::Type::Exclamation, .value = "=", .start = start });
            }
        }
        else if (next == '\'') {
            tokens.push_back(Token { .type = Token::Type::Quote, .value = "'", .start = start });
            m_in.get();
        }
        else {
            tokens.push_back(Token { .type = Token::Type::Garbage, .value = { next }, .start = start });
            m_in.get();
        }
    }
}

}
