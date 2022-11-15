#include "Lexer.hpp"

#include <cctype>
#include <db/sql/Parser.hpp>
#include <iostream>
#include <string>

namespace Db::Sql {

std::vector<Token> Lexer::lex() {
    std::vector<Token> tokens;

    auto consume_identifier = [&]() {
        std::string s;
        while (isalnum(m_in.peek()) || m_in.peek() == '_') {
            s += m_in.get();
        }
        return s;
    };

    auto consume_number = [&]() {
        std::string s;

        bool has_decimal = 0;

        if (m_in.peek() == '-')
            s += m_in.get();

        while (isdigit(m_in.peek()) || m_in.peek() == '.') {
            char c = m_in.get();
            if (c == '.') {
                if (has_decimal)
                    break;
                has_decimal = 1;
            }
            s += c;
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
        if (isalpha(next) || next == '_') {
            auto id = consume_identifier();

            std::vector<std::pair<std::string, Token::Type>> keyword_to_token_type = {
                { "ADD", Token::Type::KeywordAdd },
                { "ALL", Token::Type::KeywordAll },
                { "ALTER", Token::Type::KeywordAlter },
                { "AND", Token::Type::KeywordAnd },
                { "AS", Token::Type::KeywordAs },
                { "BETWEEN", Token::Type::KeywordBetween },
                { "BY", Token::Type::KeywordBy },
                { "CASE", Token::Type::KeywordCase },
                { "CHECK", Token::Type::KeywordCheck },
                { "COLUMN", Token::Type::KeywordColumn },
                { "CONSTRAINT", Token::Type::KeywordConstraint },
                { "CREATE", Token::Type::KeywordCreate },
                { "DEFAULT", Token::Type::KeywordDefault },
                { "DELETE", Token::Type::KeywordDelete },
                { "DISTINCT", Token::Type::KeywordDistinct },
                { "DROP", Token::Type::KeywordDrop },
                { "ELSE", Token::Type::KeywordElse },
                { "END", Token::Type::KeywordEnd },
                { "FROM", Token::Type::KeywordFrom },
                { "FOREIGN", Token::Type::KeywordForeign },
                { "FULL", Token::Type::KeywordFull },
                { "GROUP", Token::Type::KeywordGroup },
                { "HAVING", Token::Type::KeywordHaving },
                { "IMPORT", Token::Type::KeywordImport },
                { "IN", Token::Type::KeywordIn },
                { "INNER", Token::Type::KeywordInner },
                { "INSERT", Token::Type::KeywordInsert },
                { "INTO", Token::Type::KeywordInto },
                { "IS", Token::Type::KeywordIs },
                { "JOIN", Token::Type::KeywordJoin },
                { "KEY", Token::Type::KeywordKey },
                { "LIKE", Token::Type::KeywordLike },
                { "MATCH", Token::Type::KeywordMatch },
                { "NOT", Token::Type::KeywordNot },
                { "NULL", Token::Type::KeywordNull },
                { "ON", Token::Type::KeywordOn },
                { "OR", Token::Type::KeywordOr },
                { "ORDER", Token::Type::KeywordOrder },
                { "OUTER", Token::Type::KeywordOuter },
                { "OVER", Token::Type::KeywordOver },
                { "PARTITION", Token::Type::KeywordPartition },
                { "PRIMARY", Token::Type::KeywordPrimary },
                { "REFERENCES", Token::Type::KeywordReferences },
                { "SELECT", Token::Type::KeywordSelect },
                { "SET", Token::Type::KeywordSet },
                { "SHOW", Token::Type::KeywordShow },
                { "TABLE", Token::Type::KeywordTable },
                { "TABLES", Token::Type::KeywordTables },
                { "THEN", Token::Type::KeywordThen },
                { "TOP", Token::Type::KeywordTop },
                { "TRUNCATE", Token::Type::KeywordTruncate },
                { "UNION", Token::Type::KeywordUnion },
                { "UNIQUE", Token::Type::KeywordUnique },
                { "UPDATE", Token::Type::KeywordUpdate },
                { "VALUES", Token::Type::KeywordValues },
                { "WHEN", Token::Type::KeywordWhen },
                { "WHERE", Token::Type::KeywordWhere },
            };

            bool found = false;
            for (auto const& pair : keyword_to_token_type) {
                if (Parser::compare_case_insensitive(pair.first, id)) {
                    tokens.push_back(Token { .type = pair.second, .value = pair.first, .start = start });
                    found = true;
                    break;
                }
            }

            if (found) {
            }
            else if (Db::Sql::Parser::compare_case_insensitive(id, "true") || Db::Sql::Parser::compare_case_insensitive(id, "false")) {
                tokens.push_back(Token { .type = Token::Type::Bool, .value = id, .start = start });
            }
            else if (Db::Sql::Parser::compare_case_insensitive(id, "ASC") || Db::Sql::Parser::compare_case_insensitive(id, "DESC")) {
                tokens.push_back(Token { .type = Token::Type::OrderByParam, .value = id, .start = start });
            }
            else {
                tokens.push_back(Token { .type = Token::Type::Identifier, .value = id, .start = start });
            }
        }
        else if (isspace(next)) {
            m_in >> std::ws;
        }
        else if (std::isdigit(next) || next == '.' || next == '-') {
            auto number = consume_number();

            if (number == ".") {
                tokens.push_back(Token { .type = Token::Type::Period, .value = ".", .start = start });
                continue;
            }
            else if (number == "-") {
                tokens.push_back(Token { .type = Token::Type::OpSub, .value = "-", .start = start });
                continue;
            }

            Token::Type type = Token::Type::Int;

            if (number.find(".") != std::string::npos) {
                type = Token::Type::Float;
            }

            tokens.push_back(Token { .type = type, .value = number, .start = start });
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
            m_in >> std::ws;
            std::string id;
            while (m_in.peek() != ']')
                id += m_in.get();
            m_in.get(); // closing ']'

            // Strip trailing spaces
            while (isspace(id.back()))
                id.pop_back();

            tokens.push_back(Token { .type = Token::Type::Identifier, .value = id, .start = start });
        }
        else if (next == ';') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::Semicolon, .value = ";", .start = start });
        }
        else if (next == '+') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpAdd, .value = "+", .start = start });
        }
        else if (next == '-') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpSub, .value = "-", .start = start });
        }
        else if (next == '*') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpMul, .value = "*", .start = start });
        }
        else if (next == '/') {
            m_in.get();
            tokens.push_back(Token { .type = Token::Type::OpDiv, .value = "/", .start = start });
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
                tokens.push_back(Token { .type = Token::Type::Exclamation, .value = "!", .start = start });
            }
        }
        else if (next == '\'') {
            m_in.get();

            std::string id;
            while (m_in.peek() != '\'')
                id += m_in.get();
            m_in.get();

            tokens.push_back(Token { .type = Token::Type::String, .value = id, .start = start });
        }
        else if (next == '#') {
            m_in.get();
            m_in >> std::ws;
            std::string id;
            while (m_in.peek() != '#')
                id += m_in.get();
            m_in.get();

            tokens.push_back(Token { .type = Token::Type::Date, .value = id, .start = start });
        }
        else {
            tokens.push_back(Token { .type = Token::Type::Garbage, .value = { next }, .start = start });
            m_in.get();
        }
    }
}

}
