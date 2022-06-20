#include "Parser.hpp"
#include "db/core/Column.hpp"
#include "db/core/Expression.hpp"
#include "db/core/Function.hpp"
#include "db/core/Select.hpp"
#include "db/core/Table.hpp"
#include "db/core/Value.hpp"
#include "db/sql/Lexer.hpp"

#include <db/core/AST.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Db::Sql {

bool Parser::compare_case_insensitive(const std::string& lhs, const std::string& rhs) {
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

Core::DbErrorOr<std::unique_ptr<Core::AST::Statement>> Parser::parse_statement() {
    auto keyword = m_tokens[m_offset];
    // std::cout << keyword.value << "\n";
    if (keyword.type == Token::Type::KeywordSelect) {
        auto lhs = TRY(parse_select());

        if (m_tokens[m_offset].type == Token::Type::KeywordUnion) {
            m_offset++;

            bool distinct = true;

            if (m_tokens[m_offset].type == Token::Type::KeywordAll) {
                m_offset++;
                distinct = false;
            }

            if (m_tokens[m_offset].type != Token::Type::KeywordSelect)
                return expected("'SELECT' after 'UNION' statement", m_tokens[m_offset], m_offset);

            auto rhs = TRY(parse_select());

            return std::make_unique<Core::AST::Union>(std::move(lhs), std::move(rhs), distinct);
        }
        else {
            return lhs;
        }
    }
    else if (keyword.type == Token::Type::KeywordCreate) {
        auto what_to_create = m_tokens[m_offset + 1];
        if (what_to_create.type == Token::Type::KeywordTable)
            return TRY(parse_create_table());
        return expected("thing to create", what_to_create, m_offset + 1);
    }
    else if (keyword.type == Token::Type::KeywordDrop) {
        auto what_to_drop = m_tokens[m_offset + 1];
        if (what_to_drop.type == Token::Type::KeywordTable)
            return TRY(parse_drop_table());
        return expected("thing to drop", what_to_drop, m_offset + 1);
    }
    else if (keyword.type == Token::Type::KeywordTruncate) {
        auto what_to_truncate = m_tokens[m_offset + 1];
        if (what_to_truncate.type == Token::Type::KeywordTable)
            return TRY(parse_truncate_table());
        return expected("thing to truncate", what_to_truncate, m_offset + 1);
    }
    else if (keyword.type == Token::Type::KeywordAlter) {
        auto what_to_alter = m_tokens[m_offset + 1];
        if (what_to_alter.type == Token::Type::KeywordTable)
            return TRY(parse_alter_table());
        return expected("thing to alter", what_to_alter, m_offset + 1);
    }
    else if (keyword.type == Token::Type::KeywordDelete) {
        return TRY(parse_delete_from());
    }
    else if (keyword.type == Token::Type::KeywordInsert) {
        auto into_token = m_tokens[m_offset + 1];
        if (into_token.type == Token::Type::KeywordInto)
            return TRY(parse_insert_into());
        return expected("'INTO' after 'INSERT'", into_token, m_offset + 1);
    }
    else if (keyword.type == Token::Type::KeywordUpdate) {
        return TRY(parse_update());
    }
    else if (keyword.type == Token::Type::KeywordImport) {
        return TRY(parse_import());
    }
    return expected("statement", keyword, m_offset);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Select>> Parser::parse_select() {
    auto start = m_offset;
    m_offset++;

    bool distinct = false;

    if (m_tokens[m_offset].type == Token::Type::KeywordDistinct) {
        m_offset++;
        distinct = true;
    }

    std::optional<Core::AST::Top> top;
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
            columns.push_back(Core::AST::SelectColumns::Column { .alias = std::move(alias), .column = std::move(expression) });

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }
    }
    else {
        m_offset++;
    }

    // INTO
    std::optional<std::string> select_into;
    auto into = m_tokens[m_offset];
    if (into.type == Token::Type::KeywordInto) {
        m_offset++;
        auto table = m_tokens[m_offset++];

        if (table.type != Token::Type::Identifier)
            return expected("table name after 'INTO'", table, m_offset - 1);

        select_into = table.value;
    }

    // FROM
    std::optional<std::string> from_table;
    auto from = m_tokens[m_offset++];
    if (from.type == Token::Type::KeywordFrom) {
        auto from_token = m_tokens[m_offset++];
        if (from_token.type != Token::Type::Identifier)
            return expected("table name after 'FROM'", from_token, m_offset - 1);
        from_table = from_token.value;
    }

    // WHERE
    std::unique_ptr<Core::AST::Expression> where;
    if (m_tokens[m_offset].type == Token::Type::KeywordWhere) {
        m_offset++;
        where = TRY(parse_expression());
        // std::cout << "WHERE " << where->to_string() << std::endl;
        // std::cout << "~~~ " << m_tokens[m_offset].value << std::endl;
    }

    // GROUP BY
    std::optional<Core::AST::GroupBy> group;
    if (m_tokens[m_offset].type == Token::Type::KeywordGroup) {
        m_offset++;
        if (m_tokens[m_offset++].type != Token::Type::KeywordBy)
            return expected("'BY' after 'GROUP", m_tokens[m_offset], m_offset - 1);

        Core::AST::GroupBy group_by;

        while (true) {
            auto expression = TRY(parse_expression());

            group_by.columns.push_back(expression->to_string());

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }

        group = group_by;
    }

    // HAVING
    std::unique_ptr<Core::AST::Expression> having;
    if (m_tokens[m_offset].type == Token::Type::KeywordHaving) {
        m_offset++;
        having = TRY(parse_expression());
        // std::cout << "WHERE " << where->to_string() << std::endl;
        // std::cout << "~~~ " << m_tokens[m_offset].value << std::endl;
    }

    // ORDER BY
    std::optional<Core::AST::OrderBy> order;
    if (m_tokens[m_offset].type == Token::Type::KeywordOrder) {
        m_offset++;
        if (m_tokens[m_offset++].type != Token::Type::KeywordBy)
            return expected("'BY' after 'ORDER", m_tokens[m_offset], m_offset - 1);

        Core::AST::OrderBy order_by;

        while (true) {
            auto expression = TRY(parse_expression_or_index());

            auto param = m_tokens[m_offset];
            auto order_method = Core::AST::OrderBy::Order::Ascending;
            if (param.type == Token::Type::OrderByParam) {
                if (param.value == "ASC")
                    order_method = Core::AST::OrderBy::Order::Ascending;
                else
                    order_method = Core::AST::OrderBy::Order::Descending;
                m_offset++;
            }

            order_by.columns.push_back(Core::AST::OrderBy::OrderBySet { .column = std::move(expression), .order = order_method });

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }

        order = std::move(order_by);
    }

    return std::make_unique<Core::AST::Select>(start,
        Core::AST::Select::SelectOptions {
            .columns = Core::AST::SelectColumns { std::move(columns) },
            .from = from_table,
            .where = std::move(where),
            .order_by = std::move(order),
            .top = std::move(top),
            .group_by = std::move(group),
            .having = std::move(having),
            .distinct = distinct,
            .select_into = std::move(select_into) });
}

static bool is_literal(Token::Type token) {
    switch (token) {
    case Token::Type::Int:
    case Token::Type::Float:
    case Token::Type::String:
    case Token::Type::Bool:
    case Token::Type::Date:
    case Token::Type::Null:
        return true;
    default:
        return false;
    }
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Update>> Parser::parse_update() {
    auto start = m_offset;
    m_offset++;

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name after 'UPDATE'", table_name, m_offset - 1);

    std::vector<Core::AST::Update::UpdatePair> to_update;

    while (true) {
        auto set_identifier = m_tokens[m_offset++];

        if (set_identifier.type != Token::Type::KeywordSet)
            return expected("'SET'", set_identifier, m_offset - 1);

        auto column = m_tokens[m_offset++];

        if (column.type != Token::Type::Identifier)
            return expected("column name", set_identifier, m_offset - 1);

        auto equal = m_tokens[m_offset++];

        if (equal.type != Token::Type::OpEqual)
            return expected("'='", set_identifier, m_offset - 1);

        auto expr = TRY(parse_expression());

        to_update.push_back(Core::AST::Update::UpdatePair { .column = std::move(column.value), .expr = std::move(expr) });

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    return std::make_unique<Core::AST::Update>(start, table_name.value, std::move(to_update));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Import>> Parser::parse_import() {
    m_offset++; // IMPORT

    auto mode_token = m_tokens[m_offset++];
    if (mode_token.type != Token::Type::Identifier) {
        return expected("mode ('CSV')", mode_token, m_offset - 1);
    }

    Core::AST::Import::Mode mode = TRY([&]() -> Core::DbErrorOr<Core::AST::Import::Mode> {
        if (compare_case_insensitive(mode_token.value, "CSV"))
            return Core::AST::Import::Mode::Csv;
        return Core::DbError { "Invalid import mode", m_offset - 1 };
    }());

    auto file_name = m_tokens[m_offset++];
    if (file_name.type != Token::Type::String) {
        return expected("file name (string)", file_name, m_offset - 1);
    }

    auto into_token = m_tokens[m_offset++];
    if (into_token.type != Token::Type::KeywordInto) {
        return expected("'INTO'", into_token, m_offset - 1);
    }

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier) {
        return expected("table name", table_name, m_offset - 1);
    }

    return std::make_unique<Core::AST::Import>(m_offset, mode, file_name.value, table_name.value);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::DeleteFrom>> Parser::parse_delete_from() {
    auto start = m_offset;
    m_offset++;

    // FROM
    auto from = m_tokens[m_offset++];
    if (from.type != Token::Type::KeywordFrom)
        return expected("'FROM'", from, m_offset - 1);

    auto from_token = m_tokens[m_offset++];
    if (from_token.type != Token::Type::Identifier)
        return expected("table name after 'FROM'", from, m_offset - 1);

    // WHERE
    std::unique_ptr<Core::AST::Expression> where;
    if (m_tokens[m_offset].type == Token::Type::KeywordWhere) {
        m_offset++;
        where = TRY(parse_expression());
        // std::cout << "WHERE " << where->to_string() << std::endl;
        // std::cout << "~~~ " << m_tokens[m_offset].value << std::endl;
    }

    return std::make_unique<Core::AST::DeleteFrom>(start,
        from_token.value,
        std::move(where));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::CreateTable>> Parser::parse_create_table() {
    auto start = m_offset;
    m_offset += 2; // CREATE TABLE

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::CreateTable>(start, table_name.value, std::vector<Core::Column> {}, Core::Table::CheckConstraint{});
    m_offset++;

    std::unique_ptr<Core::AST::Expression> check_expr;
    std::optional<std::string> check_alias = {};

    std::vector<Core::Column> columns;
    while (true) {
        auto name = m_tokens[m_offset++];
        if (name.type != Token::Type::Identifier)
            return expected("column name", name, m_offset - 1);

        auto type_token = m_tokens[m_offset++];
        if (type_token.type != Token::Type::Identifier)
            return expected("column type", type_token, m_offset - 1);

        auto type = Core::Value::type_from_string(type_token.value);
        if (!type.has_value())
            return Core::DbError { "Invalid type: '" + type_token.value + "'", m_offset - 1 };

        bool auto_increment = false;
        bool unique = false;
        bool not_null = false;
        std::optional<Core::Value> default_value = {};
        
        while (true) {
            auto param = m_tokens[m_offset];
            if (param.type != Token::Type::Identifier && param.type != Token::Type::OpNot && param.type != Token::Type::KeywordDefault && param.type != Token::Type::KeywordUnique)
                break;
            m_offset++;
            if (param.value == "AUTO_INCREMENT")
                auto_increment = true;
            else if (param.type == Token::Type::KeywordUnique)
                unique = true;
            else if (param.type == Token::Type::OpNot){
                if(m_tokens[m_offset].type != Token::Type::Null)
                    return Core::DbError { "Expected 'NULL' after 'NOT'", m_offset };
                
                m_offset++;
                not_null = true;
            }
            else if (param.type == Token::Type::KeywordDefault){
                if(!is_literal(m_tokens[m_offset].type))
                    return Core::DbError { "Expected value after `DEFAULT`", m_offset };
                auto default_ptr = TRY(parse_literal());

                default_value = default_ptr->value();
            }
            else 
                return Core::DbError { "Invalid param for column: '" + param.value + "'", m_offset };
        }

        columns.push_back(Core::Column { name.value, *type, auto_increment, unique, not_null, std::move(default_value)});

        auto keyword_or_comma = m_tokens[m_offset];
        if(keyword_or_comma.type == Token::Type::KeywordCheck){
            m_offset++;
            check_expr = TRY(parse_expression());
        }else if(keyword_or_comma.type == Token::Type::KeywordConstraint){
            m_offset++;

            auto identifier = m_tokens[m_offset];

            if(identifier.type != Token::Type::Identifier)
                return expected("identifier", identifier, m_offset - 1);
            m_offset++;

            check_alias = identifier.value;

            if(m_tokens[m_offset].type != Token::Type::KeywordCheck)
                return expected("'CHECK' after identifier", identifier, m_offset - 1);
            m_offset++;

            check_expr = TRY(parse_expression());
        }

        if (keyword_or_comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return expected("')' to close column list", paren_close, m_offset - 1);

    return std::make_unique<Core::AST::CreateTable>(start, table_name.value, columns, Core::Table::CheckConstraint{.expr = check_expr.get(), .alias = std::move(check_alias)});
}

Core::DbErrorOr<std::unique_ptr<Core::AST::DropTable>> Parser::parse_drop_table() {
    auto start = m_offset;
    m_offset += 2; // DROP TABLE

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    return std::make_unique<Core::AST::DropTable>(start, table_name.value);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::TruncateTable>> Parser::parse_truncate_table() {
    auto start = m_offset;
    m_offset += 2; // TRUNCATE TABLE

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    return std::make_unique<Core::AST::TruncateTable>(start, table_name.value);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::AlterTable>> Parser::parse_alter_table() {
    auto start = m_offset;
    m_offset += 2; // ALTER TABLE

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    std::vector<Core::Column> to_add;
    std::vector<Core::Column> to_alter;
    std::vector<Core::Column> to_drop;

    while (m_tokens[m_offset].type == Token::Type::KeywordAdd || m_tokens[m_offset].type == Token::Type::KeywordAlter || m_tokens[m_offset].type == Token::Type::KeywordDrop) {
        if (m_tokens[m_offset].type == Token::Type::KeywordAdd) {
            m_offset++;
            auto column_token = m_tokens[m_offset++];
            if (column_token.type != Token::Type::Identifier)
                return expected("column name", column_token, m_offset - 1);

            auto type_token = m_tokens[m_offset++];
            if (type_token.type != Token::Type::Identifier)
                return expected("column type", type_token, m_offset - 1);

            // TODO: Parse autoincrement
            to_add.push_back(Core::Column(column_token.value, Core::Value::type_from_string(type_token.value).value(), 1, 1, 1));
        }
        else if (m_tokens[m_offset].type == Token::Type::KeywordAlter) {
            m_offset++;

            if (m_tokens[m_offset++].type != Token::Type::KeywordColumn)
                return expected("thing to alter", m_tokens[m_offset], m_offset - 1);

            auto column_token = m_tokens[m_offset++];
            if (column_token.type != Token::Type::Identifier)
                return expected("column name", column_token, m_offset - 1);

            auto type_token = m_tokens[m_offset++];
            if (type_token.type != Token::Type::Identifier)
                return expected("column type", type_token, m_offset - 1);

            to_alter.push_back(Core::Column(column_token.value, Core::Value::type_from_string(type_token.value).value(), 1, 1, 1));
        }
        else if (m_tokens[m_offset].type == Token::Type::KeywordDrop) {
            m_offset++;
            if (m_tokens[m_offset++].type != Token::Type::KeywordColumn)
                return expected("thing to drop", m_tokens[m_offset], m_offset - 1);

            auto column_token = m_tokens[m_offset++];
            if (column_token.type != Token::Type::Identifier)
                return expected("column name", column_token, m_offset - 1);

            to_drop.push_back(Core::Column(column_token.value, {}, 1, 1, 1));
        }
        else {
            return Core::DbError { "Unrecognized option", m_offset - 1 };
        }
    }

    return std::make_unique<Core::AST::AlterTable>(start, table_name.value, std::move(to_add), std::move(to_alter), std::move(to_drop));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::InsertInto>> Parser::parse_insert_into() {
    auto start = m_offset;
    m_offset += 2; // INSERT INTO

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::vector<std::string> {}, std::vector<std::unique_ptr<Core::AST::Expression>> {});
    m_offset++;

    std::vector<std::string> columns;
    std::vector<std::unique_ptr<Core::AST::Expression>> values;
    while (true) {
        auto name = m_tokens[m_offset++];
        if (name.type != Token::Type::Identifier)
            return expected("column name", name, m_offset - 1);

        columns.push_back(name.value);

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return expected("')' to close column list", paren_close, m_offset - 1);

    auto value_token = m_tokens[m_offset++];
    if (value_token.type == Token::Type::KeywordValues) {
        paren_open = m_tokens[m_offset];
        if (paren_open.type != Token::Type::ParenOpen)
            return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::vector<std::string> {}, std::vector<std::unique_ptr<Core::AST::Expression>> {});
        m_offset++;

        while (true) {
            values.push_back(TRY(parse_expression()));

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }

        paren_close = m_tokens[m_offset++];
        if (paren_close.type != Token::Type::ParenClose)
            return expected("')' to close values list", paren_close, m_offset - 1);
        return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::move(columns), std::move(values));
    }
    else if (value_token.type == Token::Type::KeywordSelect) {
        m_offset--;
        auto result = TRY(parse_select());
        return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::move(columns), std::move(result));
    }

    return expected("'VALUES' or 'SELECT'", value_token, m_offset - 1);
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
    else if (token.type == Token::Type::KeywordCase) {
        m_offset++;
        std::vector<Core::AST::CaseExpression::CasePair> cases;
        std::unique_ptr<Core::AST::Expression> else_value;
        while (true) {
            auto postfix = m_tokens[m_offset];

            if (postfix.type == Token::Type::KeywordWhen) {
                m_offset++;

                if (else_value)
                    return expected("'END' after 'ELSE'", token, start);

                std::unique_ptr<Core::AST::Expression> expr = TRY(parse_expression());

                auto then_expression = m_tokens[m_offset++];

                if (then_expression.type != Token::Type::KeywordThen)
                    return expected("'THEN'", token, start);

                std::unique_ptr<Core::AST::Expression> val = TRY(parse_expression());

                cases.push_back(Core::AST::CaseExpression::CasePair { .expr = std::move(expr), .value = std::move(val) });
            }
            else if (postfix.value == "ELSE") {
                m_offset++;

                if (else_value)
                    return expected("'END' after 'ELSE'", token, start);

                else_value = TRY(parse_expression());
            }
            else if (postfix.type == Token::Type::KeywordEnd) {
                m_offset++;

                lhs = std::make_unique<Core::AST::CaseExpression>(std::move(cases), std::move(else_value));
                break;
            }
            else {
                return expected("'WHEN', 'ELSE' or 'END'", token, start);
            }
        }
    }
    else if (is_literal(token.type)) {
        lhs = TRY(parse_literal());
    }
    else {
        return expected("expression", token, start);
    }

    auto maybe_operator = TRY(parse_operand(std::move(lhs), min_precedence));
    assert(maybe_operator);
    return maybe_operator;
}

Core::DbErrorOr<Core::AST::ExpressionOrIndex> Parser::parse_expression_or_index() {
    auto token = m_tokens[m_offset];
    if (token.type == Token::Type::Int) {
        m_offset++;
        auto index = std::stoi(token.value);
        if (index < 1) {
            return Core::DbError { "Index must be positive, " + token.value + " given", m_offset - 1 };
        }
        return Core::AST::ExpressionOrIndex { static_cast<size_t>(index - 1) };
    }
    return Core::AST::ExpressionOrIndex { TRY(parse_expression()) };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Literal>> Parser::parse_literal() {
    auto token = m_tokens[m_offset];
    auto start = m_offset;

    if (token.type == Token::Type::Int) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::create_int(std::stoi(token.value)));
    }
    else if (token.type == Token::Type::Float) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::create_float(std::stof(token.value)));
    }
    else if (token.type == Token::Type::String) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::create_varchar(token.value));
    }
    else if (token.type == Token::Type::Bool) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::create_bool((token.value == "true") ? 1 : 0));
    }
    else if (token.type == Token::Type::Date) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::create_time(token.value, Util::Clock::Format::NO_CLOCK_AMERICAN));
    }
    else if (token.type == Token::Type::Null) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::null());
    }

    return expected("literal", m_tokens[m_offset], m_offset - 1);
}

static int operator_precedence(Token::Type op) {
    switch (op) {
    case Token::Type::OpEqual:
    case Token::Type::OpNotEqual:
    case Token::Type::OpGreater:
    case Token::Type::OpLess:
    case Token::Type::OpLike:
        return 500;
    case Token::Type::KeywordBetween:
    case Token::Type::KeywordIn:
        return 200;
    case Token::Type::OpAnd:
        return 150;
    case Token::Type::OpOr:
        return 100;
    case Token::Type::OpMul:
    case Token::Type::OpDiv:
        return 15;
    case Token::Type::OpAdd:
    case Token::Type::OpSub:
        return 10;
    default:
        return 100000;
    }
}

Core::DbErrorOr<std::unique_ptr<Parser::BetweenRange>> Parser::parse_between_range() {
    auto min = TRY(parse_expression(operator_precedence(Token::Type::KeywordBetween) + 1));

    if (m_tokens[m_offset++].type != Token::Type::OpAnd)
        return expected("'AND' in 'BETWEEN'", m_tokens[m_offset], m_offset - 1);

    auto max = TRY(parse_expression(operator_precedence(Token::Type::KeywordBetween) + 1));

    return std::make_unique<BetweenRange>(std::move(min), std::move(max));
}

static bool is_binary_operator(Token::Type op) {
    switch (op) {
    case Token::Type::OpEqual:
    case Token::Type::OpNotEqual:
    case Token::Type::OpGreater:
    case Token::Type::OpLess:
    case Token::Type::OpLike:
    case Token::Type::KeywordBetween:
    case Token::Type::KeywordIn:
    case Token::Type::OpAnd:
    case Token::Type::OpOr:
        return true;
    default:
        return false;
    }
}

static bool is_arithmetic_operator(Token::Type op) {
    switch (op) {
    case Token::Type::OpAdd:
    case Token::Type::OpSub:
    case Token::Type::Asterisk:
    case Token::Type::OpMul:
    case Token::Type::OpDiv:
        return true;
    default:
        return false;
    }
}

static Core::AST::BinaryOperator::Operation token_type_to_binary_operation(Token::Type op) {
    switch (op) {
    case Token::Type::OpEqual:
    case Token::Type::KeywordIs:
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

static Core::AST::ArithmeticOperator::Operation token_type_to_arithmetic_operation(Token::Type op) {
    switch (op) {
    case Token::Type::OpAdd:
        return Core::AST::ArithmeticOperator::Operation::Add;
        break;
    case Token::Type::OpSub:
        return Core::AST::ArithmeticOperator::Operation::Sub;
        break;
    case Token::Type::OpMul:
    case Token::Type::Asterisk:
        return Core::AST::ArithmeticOperator::Operation::Mul;
        break;
    case Token::Type::OpDiv:
        return Core::AST::ArithmeticOperator::Operation::Div;
        break;
    default:
        return Core::AST::ArithmeticOperator::Operation::Invalid;
    }
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_operand(std::unique_ptr<Core::AST::Expression> lhs, int min_precedence) {
    auto peek_operator = [this]() {
        return m_tokens[m_offset].type;
    };

    auto current_operator = peek_operator();
    if (!is_binary_operator(current_operator) && !is_arithmetic_operator(current_operator))
        return lhs;

    while (true) {
        // std::cout << "1. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;
        auto current_operator = peek_operator();
        if (!is_binary_operator(current_operator) && !is_arithmetic_operator(current_operator))
            return lhs;
        if (operator_precedence(current_operator) < min_precedence)
            return lhs;
        m_offset++;

        // std::cout << "2. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;
        //  The "x AND y" part of BetweenExpression is treated as rhs. When right-merging
        //  only the "y" is used as rhs for recursive parse_operand. The "x" operand is handled
        //  entirely by BETWEEN.
        auto current_precedence = operator_precedence(current_operator);
        std::unique_ptr<Core::AST::Expression> rhs;

        if (current_operator == Token::Type::KeywordBetween)
            rhs = TRY(parse_between_range());
        else if (current_operator == Token::Type::KeywordIn)
            rhs = TRY(parse_in());
        else
            rhs = TRY(parse_expression(current_precedence));
        // std::cout << "3. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;

        auto next_operator = peek_operator();

        auto next_precedence = operator_precedence(next_operator);

        if (current_precedence > next_precedence) {
            if (current_operator == Token::Type::KeywordBetween) {
                auto& rhs_between_range = static_cast<BetweenRange&>(*rhs);
                lhs = std::make_unique<Core::AST::BetweenExpression>(std::move(lhs), std::move(rhs_between_range.min), std::move(rhs_between_range.max));
            }
            else if (current_operator == Token::Type::KeywordIn) {
                auto& rhs_in_args = static_cast<InArgs&>(*rhs);
                lhs = std::make_unique<Core::AST::InExpression>(std::move(lhs), std::move(rhs_in_args.args));
            }
            else if (is_binary_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), token_type_to_binary_operation(current_operator), std::move(rhs));
            }
            else if (is_arithmetic_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::ArithmeticOperator>(std::move(lhs), token_type_to_arithmetic_operation(current_operator), std::move(rhs));
            }
        }
        else {
            if (current_operator == Token::Type::KeywordBetween) {
                auto& rhs_between_range = static_cast<BetweenRange&>(*rhs);
                lhs = std::make_unique<Core::AST::BetweenExpression>(std::move(lhs), std::move(rhs_between_range.min), TRY(parse_operand(std::move(rhs_between_range.max))));
            }
            else if (current_operator == Token::Type::KeywordIn) {
                auto& rhs_in_args = static_cast<InArgs&>(*rhs);
                lhs = std::make_unique<Core::AST::InExpression>(std::move(lhs), std::move(rhs_in_args.args));
            }
            else if (is_binary_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), token_type_to_binary_operation(current_operator), TRY(parse_operand(std::move(rhs))));
            }
            else if (is_arithmetic_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::ArithmeticOperator>(std::move(lhs), token_type_to_arithmetic_operation(current_operator), TRY(parse_operand(std::move(rhs))));
            }
        }
    }
}

Core::AST::AggregateFunction::Function to_aggregate_function(std::string const& name) {
    // TODO: Case-insensitive match
    if (Parser::compare_case_insensitive(name, "COUNT"))
        return Core::AST::AggregateFunction::Function::Count;
    else if (Parser::compare_case_insensitive(name, "SUM"))
        return Core::AST::AggregateFunction::Function::Sum;
    else if (Parser::compare_case_insensitive(name, "MIN"))
        return Core::AST::AggregateFunction::Function::Min;
    else if (Parser::compare_case_insensitive(name, "MAX"))
        return Core::AST::AggregateFunction::Function::Max;
    else if (Parser::compare_case_insensitive(name, "AVG"))
        return Core::AST::AggregateFunction::Function::Avg;
    return Core::AST::AggregateFunction::Function::Invalid;
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_function(std::string name) {
    auto start = m_offset - 1;
    m_offset++; // (

    std::vector<std::unique_ptr<Core::AST::Expression>> args;
    while (true) {
        // std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;

        auto aggregate_function = to_aggregate_function(name);
        if (aggregate_function != Core::AST::AggregateFunction::Function::Invalid) {
            auto column = m_tokens[m_offset++];
            if (column.type != Token::Type::Identifier)
                return expected("identifier in aggregate function", column, m_offset - 1);

            if (m_tokens[m_offset++].type != Token::Type::ParenClose)
                return expected("')' to close aggregate function", m_tokens[m_offset], m_offset - 1);

            return { std::make_unique<Core::AST::AggregateFunction>(m_offset, aggregate_function, column.value) };
        }

        auto expression = TRY(parse_expression());
        args.push_back(std::move(expression));

        auto comma_or_paren_close = m_tokens[m_offset];
        if (comma_or_paren_close.type != Token::Type::Comma) {
            if (comma_or_paren_close.type != Token::Type::ParenClose)
                return expected("')' to close function", comma_or_paren_close, m_offset);
            m_offset++;
            break;
        }
        m_offset++;
    }
    return std::make_unique<Core::AST::Function>(start, std::move(name), std::move(args));
}

Core::DbErrorOr<std::unique_ptr<Parser::InArgs>> Parser::parse_in() {
    std::vector<std::unique_ptr<Core::AST::Expression>> args;
    auto paren_open = m_tokens[m_offset++];

    if (paren_open.type != Token::Type::ParenOpen)
        return expected("'('", paren_open, m_offset - 1);

    while (true) {
        auto expression = TRY(parse_expression());
        args.push_back(std::move(expression));

        auto comma_or_paren_close = m_tokens[m_offset];
        if (comma_or_paren_close.type != Token::Type::Comma) {
            if (comma_or_paren_close.type != Token::Type::ParenClose)
                return expected("')' to close IN expression", comma_or_paren_close, m_offset);
            m_offset++;
            break;
        }
        m_offset++;
    }
    return std::make_unique<Parser::InArgs>(std::move(args));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Identifier>> Parser::parse_identifier() {
    auto name = m_tokens[m_offset++];
    if (name.type != Token::Type::Identifier)
        return expected("identifier", name, m_offset - 1);

    return std::make_unique<Core::AST::Identifier>(m_offset - 1, name.value);
}

Core::DbError Parser::expected(std::string what, Token got, size_t offset) {
    return Core::DbError { "Expected " + what + ", got '" + got.value + "'", offset };
}

}
