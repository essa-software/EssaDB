#include "Parser.hpp"

#include <db/core/AST.hpp>

#include <iostream>

namespace Db::Sql {

Core::DbErrorOr<std::unique_ptr<Core::AST::Statement>> Parser::parse_statement() {
    auto keyword = m_tokens[m_offset];
    if (keyword.type == Token::Type::KeywordSelect) {
        return TRY(parse_select());
    }
    return Core::DbError { "Expected statement, got '" + keyword.value + '"' };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Select>> Parser::parse_select() {
    auto select = m_tokens[m_offset++];
    if (select.type != Token::Type::KeywordSelect)
        return Core::DbError { "Expected 'SELECT'" };

    std::optional<Core::AST::Top> top;

    if (m_tokens[m_offset].type == Token::Type::KeywordAfterSelect) {
        if (m_tokens[m_offset++].value == "TOP") {
            try {
                unsigned value = std::stoi(m_tokens[m_offset++].value);
                if (m_tokens[m_offset].value == "PERC") {
                    top = Core::AST::Top { .unit = Core::AST::Top::Unit::Perc, .value = value };

                    m_offset++;
                }
                else
                    top = Core::AST::Top { .unit = Core::AST::Top::Unit::Val, .value = value };
            } catch (...) {
                return Core::DbError { "Invalid argument for TOP" };
            }
        }
    }

    std::vector<std::unique_ptr<Core::AST::Expression>> columns;

    auto maybe_asterisk = m_tokens[m_offset];
    if (maybe_asterisk.type != Token::Type::Asterisk) {
        while (true) {
            std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

            auto expression = TRY(parse_expression());
            columns.push_back(std::move(expression));

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }
    }
    else {
        m_offset++;
    }

    auto from = m_tokens[m_offset++];
    if (from.type != Token::Type::KeywordFrom)
        return Core::DbError { "Expected 'FROM', got " + from.value };

    auto from_token = m_tokens[m_offset++];
    if (from_token.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name after 'FROM'" };

    return std::make_unique<Core::AST::Select>(Core::AST::SelectColumns { std::move(columns) },
        from_token.value,
        std::optional<Core::AST::Filter> {},
        std::optional<Core::AST::OrderBy> {},
        std::move(top));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_expression() {
    auto token = m_tokens[m_offset++];
    if (token.type == Token::Type::Identifier) {
        auto postfix = m_tokens[m_offset];
        if (postfix.type == Token::Type::ParenOpen)
            return TRY(parse_function(std::move(token.value)));
        else
            return std::make_unique<Core::AST::Identifier>(token.value);
    }
    std::cout << m_offset << std::endl;
    return Core::DbError { "Expected expression, got '" + token.value + "'" };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Function>> Parser::parse_function(std::string name) {
    m_offset++; // (

    std::vector<std::unique_ptr<Core::AST::Expression>> args;
    while (true) {
        std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

        auto expression = TRY(parse_expression());
        args.push_back(std::move(expression));

        auto comma_or_paren_close = m_tokens[m_offset];
        if (comma_or_paren_close.type != Token::Type::Comma) {
            if (comma_or_paren_close.type != Token::Type::ParenClose)
                return Core::DbError { "Expected ')' to close function" };
            m_offset++;
            break;
        }
        m_offset++;
    }
    return std::make_unique<Core::AST::Function>(std::move(name), std::move(args));
}

}
