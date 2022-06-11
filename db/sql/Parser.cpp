#include "Parser.hpp"
#include "db/core/Value.hpp"
#include "db/sql/Lexer.hpp"

#include <db/core/AST.hpp>

#include <iostream>
#include <string>
#include <utility>

namespace Db::Sql {

Core::DbErrorOr<std::unique_ptr<Core::AST::Statement>> Parser::parse_statement() {
    auto keyword = m_tokens[m_offset];
    if (keyword.type == Token::Type::KeywordSelect) {
        return TRY(parse_select());
    }
    else if (keyword.type == Token::Type::KeywordCreate) {
        auto what_to_create = m_tokens[m_offset + 1];
        if (what_to_create.type == Token::Type::KeywordTable)
            return TRY(parse_create_table());
        return Core::DbError { "Expected thing to create, got '" + what_to_create.value + "'", m_offset + 1 };
    }
    else if (keyword.type == Token::Type::KeywordInsert) {
        auto into_token = m_tokens[m_offset + 1];
        if (into_token.type == Token::Type::KeywordInto)
            return TRY(parse_insert_into());
        return Core::DbError { "Expected keyword 'INTO', got '" + into_token.value + "'", m_offset + 1 };
    }
    return Core::DbError { "Expected statement, got '" + keyword.value + '"', m_offset };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Select>> Parser::parse_select() {
    auto start = m_offset;

    auto select = m_tokens[m_offset++];
    if (select.type != Token::Type::KeywordSelect)
        return Core::DbError { "Expected 'SELECT'", start };

    std::optional<Core::AST::Top> top;
    std::optional<Core::AST::OrderBy> order;

    if (m_tokens[m_offset].type == Token::Type::KeywordTop) {
        m_offset++;
        try {
            unsigned value = std::stoi(m_tokens[m_offset++].value);
            if (m_tokens[m_offset].value == "PERC") {
                top = Core::AST::Top { .unit = Core::AST::Top::Unit::Perc, .value = value };

                m_offset++;
            }
            else
                top = Core::AST::Top { .unit = Core::AST::Top::Unit::Val, .value = value };
        } catch (...) {
            return Core::DbError { "Invalid argument for TOP", m_offset };
        }
    }

    std::vector<Core::AST::SelectColumns::Column> columns;

    auto maybe_asterisk = m_tokens[m_offset];
    if (maybe_asterisk.type != Token::Type::Asterisk) {
        while (true) {
            // std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

            auto expression = TRY(parse_expression());
            std::optional<std::string> alias;

            if (m_tokens[m_offset].type == Token::Type::KeywordAlias) {
                m_offset++;
                alias = m_tokens[m_offset++].value;
            }

            assert(expression);
            columns.push_back(Core::AST::SelectColumns::Column { .column = std::move(expression), .alias = std::move(alias) });

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }
    }
    else {
        m_offset++;
    }

    // FROM
    auto from = m_tokens[m_offset++];
    if (from.type != Token::Type::KeywordFrom)
        return Core::DbError { "Expected 'FROM', got " + from.value, m_offset - 1 };

    auto from_token = m_tokens[m_offset++];
    if (from_token.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name after 'FROM'", m_offset - 1 };

    // WHERE
    std::unique_ptr<Core::AST::Expression> where;
    if (m_tokens[m_offset].type == Token::Type::KeywordWhere) {
        m_offset++;
        where = TRY(parse_expression());
        // std::cout << "WHERE " << where->to_string() << std::endl;
        // std::cout << "~~~ " << m_tokens[m_offset].value << std::endl;
    }

    // ORDER BY
    if (m_tokens[m_offset].type == Token::Type::KeywordOrder) {
        m_offset++;
        if (m_tokens[m_offset++].type != Token::Type::KeywordBy)
            return Core::DbError { "Expected 'BY' after 'ORDER'", m_offset - 1 };

        Core::AST::OrderBy order_by;

        while (true) {
            auto expression = TRY(parse_expression());

            auto param = m_tokens[m_offset];
            auto order_method = Core::AST::OrderBy::Order::Ascending;
            if (param.type == Token::Type::OrderByParam) {
                if (param.value == "ASC")
                    order_method = Core::AST::OrderBy::Order::Ascending;
                else
                    order_method = Core::AST::OrderBy::Order::Descending;
                m_offset++;
            }

            order_by.columns.push_back(Core::AST::OrderBy::OrderBySet { .name = expression->to_string(), .order = order_method });

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }

        order = order_by;
    }

    if (m_tokens[m_offset].type != Token::Type::Semicolon)
        return Core::DbError { "Expected ';', got '" + m_tokens[m_offset].value + "'", m_offset };

    return std::make_unique<Core::AST::Select>(start, Core::AST::SelectColumns { std::move(columns) },
        from_token.value,
        std::move(where),
        std::move(order),
        std::move(top));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::CreateTable>> Parser::parse_create_table() {
    auto start = m_offset;
    m_offset += 2; // CREATE TABLE

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name", m_offset - 1 };

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::CreateTable>(start, table_name.value, std::vector<Core::Column> {});
    m_offset++;

    std::vector<Core::Column> columns;
    while (true) {
        auto name = m_tokens[m_offset++];
        if (name.type != Token::Type::Identifier)
            return Core::DbError { "Expected column name", m_offset - 1 };

        auto type_token = m_tokens[m_offset++];
        if (type_token.type != Token::Type::Identifier)
            return Core::DbError { "Expected column type", m_offset - 1 };

        auto type = Core::Value::type_from_string(type_token.value);
        if (!type.has_value())
            return Core::DbError { "Invalid type: '" + type_token.value + "'", m_offset - 1 };

        columns.push_back(Core::Column { name.value, *type });

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return Core::DbError { "Expected ')' to close column list", m_offset - 1 };

    return std::make_unique<Core::AST::CreateTable>(start, table_name.value, columns);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::InsertInto>> Parser::parse_insert_into() {
    auto start = m_offset;
    m_offset += 2; // INSERT INTO

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name", m_offset - 1 };

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::vector<std::string> {}, std::vector<std::unique_ptr<Core::AST::Expression>> {});
    m_offset++;

    std::vector<std::string> columns;
    while (true) {
        auto name = m_tokens[m_offset++];
        if (name.type != Token::Type::Identifier)
            return Core::DbError { "Expected column name", m_offset - 1 };

        columns.push_back(name.value);

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return Core::DbError { "Expected ')' to close column list", m_offset - 1 };

    auto value_token = m_tokens[m_offset++];
    if (value_token.type != Token::Type::KeywordValues)
        return Core::DbError { "Expected keyword 'VALUES'", m_offset - 1 };

    paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::vector<std::string> {}, std::vector<std::unique_ptr<Core::AST::Expression>> {});
    m_offset++;

    std::vector<std::unique_ptr<Core::AST::Expression>> values;
    while (true) {
        values.push_back(TRY(parse_expression()));

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return Core::DbError { "Expected ')' to close values list", m_offset - 1 };

    return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::move(columns), std::move(values));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_expression(int min_precedence) {
    std::unique_ptr<Core::AST::Expression> lhs;

    auto start = m_offset;
    auto token = m_tokens[m_offset];
    // std::cout << "parse_expression " << token.value << std::endl;
    if (token.type == Token::Type::Identifier) {
        auto postfix = m_tokens[m_offset + 1];
        if (postfix.type == Token::Type::ParenOpen) {
            m_offset++;
            lhs = TRY(parse_function(std::move(token.value)));
        }
        else {
            lhs = TRY(parse_identifier());
        }
    }
    else if (token.type == Token::Type::Number) {
        m_offset++;
        lhs = std::make_unique<Core::AST::Literal>(start, Core::Value::create_int(std::stoi(token.value)));
    }
    else {
        return Core::DbError { "Expected expression, got '" + token.value + "'", start };
    }

    auto maybe_operator = TRY(parse_operand(std::move(lhs), min_precedence));
    assert(maybe_operator);
    return maybe_operator;
}

static int operator_precedence(Token::Type op) {
    switch (op) {
    case Token::Type::OpEqual:
    case Token::Type::OpNotEqual:
    case Token::Type::OpGreater:
    case Token::Type::OpLess:
    case Token::Type::OpLike:
        return 50;
    case Token::Type::KeywordBetween:
        return 20;
    case Token::Type::OpAnd:
        return 15;
    case Token::Type::OpOr:
        return 10;
    default:
        return 100000;
    }
}

Core::DbErrorOr<std::unique_ptr<Parser::BetweenRange>> Parser::parse_between_range() {
    auto min = TRY(parse_expression(operator_precedence(Token::Type::KeywordBetween) + 1));

    if (m_tokens[m_offset++].type != Token::Type::OpAnd)
        return Core::DbError { "Expected 'AND' in 'BETWEEN', got '" + m_tokens[m_offset].value + "'", m_offset - 1 };

    auto max = TRY(parse_expression(operator_precedence(Token::Type::KeywordBetween) + 1));

    return std::make_unique<BetweenRange>(std::move(min), std::move(max));
}

static bool is_operator(Token::Type op) {
    switch (op) {
    case Token::Type::OpEqual:
    case Token::Type::OpNotEqual:
    case Token::Type::OpGreater:
    case Token::Type::OpLess:
    case Token::Type::OpLike:
    case Token::Type::KeywordBetween:
    case Token::Type::OpAnd:
    case Token::Type::OpOr:
        return true;
    default:
        return false;
    }
}

static Core::AST::BinaryOperator::Operation token_type_to_binary_operation(Token::Type op) {
    switch (op) {
    case Token::Type::OpEqual:
        return Core::AST::BinaryOperator::Operation::Equal;
        break;
    case Token::Type::OpLess:
        // TODO: <=
        return Core::AST::BinaryOperator::Operation::Less;
        break;
    case Token::Type::OpGreater:
        // TODO: >=
        return Core::AST::BinaryOperator::Operation::Greater;
        break;
    case Token::Type::OpNotEqual:
        return Core::AST::BinaryOperator::Operation::NotEqual;
        break;
    case Token::Type::OpLike:
        return Core::AST::BinaryOperator::Operation::Like;
        break;
    case Token::Type::OpAnd:
        return Core::AST::BinaryOperator::Operation::And;
        break;
    case Token::Type::OpOr:
        return Core::AST::BinaryOperator::Operation::Or;
        break;
    default:
        return Core::AST::BinaryOperator::Operation::Invalid;
    }
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_operand(std::unique_ptr<Core::AST::Expression> lhs, int min_precedence) {
    auto peek_operator = [this]() {
        return m_tokens[m_offset].type;
    };

    auto current_operator = peek_operator();
    if (!is_operator(current_operator))
        return lhs;

    while (true) {
        // std::cout << "1. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;
        auto current_operator = peek_operator();
        if (!is_operator(current_operator))
            return lhs;
        if (operator_precedence(current_operator) < min_precedence)
            return lhs;
        m_offset++;

        // std::cout << "2. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;
        //  The "x AND y" part of BetweenExpression is treated as rhs. When right-merging
        //  only the "y" is used as rhs for recursive parse_operand. The "x" operand is handled
        //  entirely by BETWEEN.
        auto current_precedence = operator_precedence(current_operator);
        auto rhs = current_operator == Token::Type::KeywordBetween ? TRY(parse_between_range()) : TRY(parse_expression(current_precedence));
        // std::cout << "3. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;

        auto next_operator = peek_operator();

        auto next_precedence = operator_precedence(next_operator);

        if (current_precedence > next_precedence) {
            if (current_operator == Token::Type::KeywordBetween) {
                auto& rhs_between_range = static_cast<BetweenRange&>(*rhs);
                lhs = std::make_unique<Core::AST::BetweenExpression>(std::move(lhs), std::move(rhs_between_range.min), std::move(rhs_between_range.max));
            }
            else {
                lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), token_type_to_binary_operation(current_operator), std::move(rhs));
            }
        }
        else {
            if (current_operator == Token::Type::KeywordBetween) {
                auto& rhs_between_range = static_cast<BetweenRange&>(*rhs);
                lhs = std::make_unique<Core::AST::BetweenExpression>(std::move(lhs), std::move(rhs_between_range.min), TRY(parse_operand(std::move(rhs_between_range.max))));
            }
            else {
                lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), token_type_to_binary_operation(current_operator), TRY(parse_operand(std::move(rhs))));
            }
        }
    }
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Function>> Parser::parse_function(std::string name) {
    auto start = m_offset - 1;
    m_offset++; // (

    std::vector<std::unique_ptr<Core::AST::Expression>> args;
    while (true) {
        // std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

        auto expression = TRY(parse_expression());
        args.push_back(std::move(expression));

        auto comma_or_paren_close = m_tokens[m_offset];
        if (comma_or_paren_close.type != Token::Type::Comma) {
            if (comma_or_paren_close.type != Token::Type::ParenClose)
                return Core::DbError { "Expected ')' to close function", m_offset };
            m_offset++;
            break;
        }
        m_offset++;
    }
    return std::make_unique<Core::AST::Function>(start, std::move(name), std::move(args));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Identifier>> Parser::parse_identifier() {
    auto name = m_tokens[m_offset++];
    if (name.type != Token::Type::Identifier)
        return Core::DbError { "Expected identifier", m_offset - 1 };

    return std::make_unique<Core::AST::Identifier>(m_offset - 1, name.value);
}
}
