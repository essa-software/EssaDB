#include "Parser.hpp"
#include "db/core/Value.hpp"
#include "db/sql/Lexer.hpp"

#include <db/core/AST.hpp>

#include <iostream>
#include <utility>

namespace Db::Sql {

Core::DbErrorOr<std::unique_ptr<Core::AST::Statement>> Parser::parse_statement() {
    auto keyword = m_tokens[m_offset];
    if (keyword.type == Token::Type::KeywordSelect) {
        return TRY(parse_select());
    }
    if (keyword.type == Token::Type::KeywordCreate) {
        m_offset++;
        auto what_to_create = m_tokens[m_offset++];
        if (what_to_create.type == Token::Type::KeywordTable)
            return TRY(parse_create_table());
        return Core::DbError { "Expected thing to create, got '" + what_to_create.value + "'" };
    }
    return Core::DbError { "Expected statement, got '" + keyword.value + '"' };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Select>> Parser::parse_select() {
    auto select = m_tokens[m_offset++];
    if (select.type != Token::Type::KeywordSelect)
        return Core::DbError { "Expected 'SELECT'" };

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
            return Core::DbError { "Invalid argument for TOP" };
        }
    }

    std::vector<Core::AST::SelectColumns::Column> columns;

    auto maybe_asterisk = m_tokens[m_offset];
    if (maybe_asterisk.type != Token::Type::Asterisk) {
        while (true) {
            // std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

            auto expression = TRY(parse_expression(AllowOperators::Yes));
            std::optional<std::string> alias;

            if (m_tokens[m_offset].type == Token::Type::KeywordAlias) {
                m_offset++;
                alias = TRY(parse_identifier())->to_string();
            }

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
        return Core::DbError { "Expected 'FROM', got " + from.value };

    auto from_token = m_tokens[m_offset++];
    if (from_token.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name after 'FROM'" };

    std::unique_ptr<Core::AST::Expression> where;

    while (true) {
        if (m_tokens[m_offset].type == Token::Type::KeywordOrder) {
            // ORDER BY
            m_offset++;
            if (m_tokens[m_offset++].type != Token::Type::KeywordBy)
                return Core::DbError { "Expected 'BY' after 'ORDER'" };

            Core::AST::OrderBy order_by;

            while (true) {
                auto expression = TRY(parse_expression(AllowOperators::Yes));

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
        }else if(m_tokens[m_offset].type == Token::Type::KeywordWhere){
            // WHERE
            m_offset++;

            where = TRY(parse_expression(AllowOperators::Yes));
        }

        if (m_tokens[m_offset].type == Token::Type::Eof)
            return Core::DbError { "Expected ';' before EOF" };
        else if (m_tokens[m_offset].type == Token::Type::Semicolon)
            break;
    }

    return std::make_unique<Core::AST::Select>(Core::AST::SelectColumns { std::move(columns) },
        from_token.value,
        std::move(where),
        std::move(order),
        std::move(top));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::CreateTable>> Parser::parse_create_table() {
    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name" };

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::CreateTable>(table_name.value, std::vector<Core::Column> {});
    m_offset++;

    std::vector<Core::Column> columns;
    while (true) {
        auto name = m_tokens[m_offset++];
        if (name.type != Token::Type::Identifier)
            return Core::DbError { "Expected column name" };

        auto type_token = m_tokens[m_offset++];
        if (type_token.type != Token::Type::Identifier)
            return Core::DbError { "Expected column type" };

        auto type = Core::Value::type_from_string(type_token.value);
        if (!type.has_value())
            return Core::DbError { "Invalid type: '" + type_token.value + "'" };

        columns.push_back(Core::Column { name.value, *type });

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return Core::DbError { "Expected ')' to close column list" };

    return std::make_unique<Core::AST::CreateTable>(table_name.value, columns);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_expression(AllowOperators allow_operators) {
    auto token = m_tokens[m_offset];
    if (token.type == Token::Type::Identifier) {
        auto postfix = m_tokens[m_offset + 1];
        if (postfix.type == Token::Type::ParenOpen) {
            m_offset++;
            return TRY(parse_function(std::move(token.value)));
        }
        else {
            auto identifier = TRY(parse_identifier());
            if (allow_operators == AllowOperators::Yes) {
                auto maybe_operator = TRY(parse_operand(std::move(identifier)));
                if (!maybe_operator)
                    return identifier;
                return maybe_operator;
            }
            else
                return identifier;
        }
    }else if (token.type == Token::Type::Quote) {
        std::string str = "";

        while(m_tokens[m_offset++ + 1].type != Token::Type::Quote){
            str += m_tokens[m_offset].value;
        }

        std::cout << str << "\n";

        return std::make_unique<Core::AST::Literal>(Core::Value::create_varchar(str));
    }
    else if (token.type == Token::Type::Number) {
        return std::make_unique<Core::AST::Literal>(Core::Value::create_int(std::stoi(token.value)));
    }
    return Core::DbError { "Expected expression, got '" + token.value + "'" };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_operand(std::unique_ptr<Core::AST::Expression> lhs) {
    while (true) {
        auto operator_token = m_tokens[m_offset++];
        Core::AST::BinaryOperator::Operation ast_operator {};
        switch (operator_token.type) {
        case Token::Type::OpEqual:
            ast_operator = Core::AST::BinaryOperator::Operation::Equal;
            break;
        case Token::Type::OpLess:
            if(m_tokens[m_offset].type == Token::Type::OpEqual){
                ast_operator = Core::AST::BinaryOperator::Operation::LessEqual;
                m_offset++;
            }else {
                ast_operator = Core::AST::BinaryOperator::Operation::Less;
            }
            break;
        case Token::Type::OpGreater:
            if(m_tokens[m_offset].type == Token::Type::OpEqual){
                ast_operator = Core::AST::BinaryOperator::Operation::GreaterEqual;
                m_offset++;
            }else {
                ast_operator = Core::AST::BinaryOperator::Operation::Greater;
            }
            break;
        case Token::Type::Exclamation:
            if(m_tokens[m_offset++].type != Token::Type::OpEqual)
                return Core::DbError { "Expected '!='" };
            ast_operator = Core::AST::BinaryOperator::Operation::NotEqual;
            break;
        case Token::Type::OpLike:
            ast_operator = Core::AST::BinaryOperator::Operation::Like;
            break;
        default:
            return lhs;
        }
        auto rhs = TRY(parse_expression(AllowOperators::No));
        lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), ast_operator, std::move(rhs));
    }
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Function>> Parser::parse_function(std::string name) {
    m_offset++; // (

    std::vector<std::unique_ptr<Core::AST::Expression>> args;
    while (true) {
        // std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

        auto expression = TRY(parse_expression(AllowOperators::Yes));
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

Core::DbErrorOr<std::unique_ptr<Core::AST::Identifier>> Parser::parse_identifier() {
    if (m_tokens[m_offset].type == Token::Type::SquaredParenOpen) {
        m_offset++;
        std::string string;
        while (true) {
            if (m_tokens[m_offset].type == Token::Type::SquaredParenClose) {
                m_offset++;
                break;
            }
            else if (m_tokens[m_offset].type != Token::Type::Identifier) {
                // FIXME: Actually anything is allowed between square brackets, maybe we
                //        should do this on the lexer side?
                return Core::DbError { "Invalid syntax, expected identifier, got \'" + m_tokens[m_offset].value + "\'" };
            }

            string += m_tokens[m_offset++].value + " ";
        }

        // Remove trailing space
        string.pop_back();
        return std::make_unique<Core::AST::Identifier>(string);
    }

    auto name = m_tokens[m_offset];
    if (name.type != Token::Type::Identifier)
        return Core::DbError { "Invalid identifier, expected `name` or `[name]`" };

    m_offset++;
    return std::make_unique<Core::AST::Identifier>(name.value);
}

}
