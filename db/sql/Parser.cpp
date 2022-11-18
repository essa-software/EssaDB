#include "Parser.hpp"

#include <db/core/Column.hpp>
#include <db/core/DbError.hpp>
#include <db/core/Function.hpp>
#include <db/core/IndexedRelation.hpp>
#include <db/core/Select.hpp>
#include <db/core/Table.hpp>
#include <db/core/Value.hpp>
#include <db/core/ast/Show.hpp>
#include <db/sql/Lexer.hpp>

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
        ssize_t start = m_offset;
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

            return std::make_unique<Core::AST::Union>(start, std::move(lhs), std::move(rhs), distinct);
        }
        else {
            return std::make_unique<Core::AST::SelectStatement>(start, std::move(lhs));
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
    else if (keyword.type == Token::Type::KeywordShow) {
        auto type = m_tokens[++m_offset];
        switch (type.type) {
        case Token::Type::KeywordTables:
            return std::make_unique<Core::AST::Show>(m_offset - 1, Core::AST::Show::Type::Tables);
        default:
            break;
        }
        return expected("'TABLES'", m_tokens[m_offset], m_offset);
    }
    return expected("statement", keyword, m_offset);
}

Core::DbErrorOr<Core::AST::StatementList> Parser::parse_statement_list() {
    std::vector<std::unique_ptr<Core::AST::Statement>> statement_list;
    while (true) {
        if (m_tokens[m_offset].type == Token::Type::Eof) {
            break;
        }
        statement_list.push_back(TRY(parse_statement()));
        if (m_tokens[m_offset++].type != Token::Type::Semicolon) {
            return expected("semicolon at the end of statement", m_tokens[m_offset - 1], m_offset - 2);
        }
    }
    return Core::AST::StatementList { static_cast<ssize_t>(statement_list[0]->start()), std::move(statement_list) };
}

Core::DbErrorOr<Core::AST::Select> Parser::parse_select() {
    auto start = m_offset;

    // SELECT
    m_offset++;

    // DISTINCT
    bool distinct = false;
    if (m_tokens[m_offset].type == Token::Type::KeywordDistinct) {
        m_offset++;
        distinct = true;
    }

    // TOP
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

    // Columns
    std::vector<Core::AST::SelectColumns::Column> columns;

    auto maybe_asterisk = m_tokens[m_offset];
    if (maybe_asterisk.type != Token::Type::Asterisk) {
        while (true) {
            // std::cout << "PARSE EXPRESSION AT " << m_offset << std::endl;
            auto expression = TRY(parse_expression());

            std::optional<std::string> alias;

            if (m_tokens[m_offset].type == Token::Type::KeywordAs) {
                m_offset++;
                if (m_tokens[m_offset].type != Token::Type::Identifier) {
                    return expected("identifier in alias", m_tokens[m_offset], m_offset);
                }
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

    Core::AST::SelectColumns select_columns { std::move(columns) };

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
    std::unique_ptr<Core::AST::TableExpression> from_table = {};
    if (m_tokens[m_offset++].type == Token::Type::KeywordFrom) {
        from_table = TRY(parse_table_expression());
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
        group_by.type = Core::AST::GroupBy::GroupOrPartition::GROUP;

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

    // PARTITION BY
    if (m_tokens[m_offset].type == Token::Type::KeywordPartition) {
        m_offset++;
        if (group)
            Core::DbError { "'PARTITION BY' can't be used with 'GROUP BY'", m_offset - 1 };

        if (m_tokens[m_offset++].type != Token::Type::KeywordBy)
            return expected("'BY' after 'GROUP", m_tokens[m_offset], m_offset - 1);

        Core::AST::GroupBy partition_by;
        partition_by.type = Core::AST::GroupBy::GroupOrPartition::PARTITION;

        while (true) {
            auto expression = TRY(parse_expression());

            partition_by.columns.push_back(expression->to_string());

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }

        group = partition_by;
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
            auto expression = TRY(parse_expression_or_index(select_columns));

            auto param = m_tokens[m_offset];
            auto order_method = Core::AST::OrderBy::Order::Ascending;
            if (param.type == Token::Type::OrderByParam) {
                if (param.value == "ASC")
                    order_method = Core::AST::OrderBy::Order::Ascending;
                else
                    order_method = Core::AST::OrderBy::Order::Descending;
                m_offset++;
            }

            order_by.columns.push_back(Core::AST::OrderBy::OrderBySet { .expression = std::move(expression), .order = order_method });

            auto comma = m_tokens[m_offset];
            if (comma.type != Token::Type::Comma)
                break;
            m_offset++;
        }

        order = std::move(order_by);
    }

    return Core::AST::Select { start,
        Core::AST::Select::SelectOptions {
            .columns = std::move(select_columns),
            .from = std::move(from_table),
            .where = std::move(where),
            .order_by = std::move(order),
            .top = std::move(top),
            .group_by = std::move(group),
            .having = std::move(having),
            .distinct = distinct,
            .select_into = std::move(select_into) } };
}

static bool is_literal(Token::Type token) {
    switch (token) {
    case Token::Type::Int:
    case Token::Type::Float:
    case Token::Type::String:
    case Token::Type::Bool:
    case Token::Type::Date:
    case Token::Type::KeywordNull:
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

Core::DbErrorOr<Core::AST::ParsedColumn> Parser::parse_column() {
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
    std::variant<std::monostate, Core::PrimaryKey, Core::ForeignKey> key;

    while (true) {
        auto param = m_tokens[m_offset];
        if (param.type != Token::Type::Identifier
            && param.type != Token::Type::KeywordDefault
            && param.type != Token::Type::KeywordForeign
            && param.type != Token::Type::KeywordNot
            && param.type != Token::Type::KeywordPrimary
            && param.type != Token::Type::KeywordUnique)
            break;
        m_offset++;
        if (param.value == "AUTO_INCREMENT")
            auto_increment = true;
        else if (param.type == Token::Type::KeywordUnique) {
            if (unique)
                return Core::DbError { "Column is already 'UNIQUE'", m_offset };

            unique = true;
        }
        else if (param.type == Token::Type::KeywordNot) {
            if (m_tokens[m_offset].type != Token::Type::KeywordNull)
                return Core::DbError { "Expected 'NULL' after 'NOT'", m_offset };
            m_offset++;

            if (not_null)
                return Core::DbError { "Column is already 'NOT NULL'", m_offset };

            not_null = true;
        }
        else if (param.type == Token::Type::KeywordDefault) {
            if (!is_literal(m_tokens[m_offset].type))
                return Core::DbError { "Expected value after `DEFAULT`", m_offset };
            auto default_ptr = TRY(parse_literal());

            if (default_value)
                return Core::DbError { "Column already has its default value!", m_offset };
            default_value = default_ptr->value();
        }
        else if (param.type == Token::Type::KeywordPrimary) {
            if (m_tokens[m_offset].type != Token::Type::KeywordKey)
                return Core::DbError { "Expected 'KEY' after 'PRIMARY'", m_offset };
            m_offset++;

            if (unique || not_null)
                return Core::DbError { "Column is already 'UNIQUE' or 'NOT NULL'", m_offset };

            unique = true;
            not_null = true;
            key = Core::PrimaryKey { .local_column = name.value };
        }
        else if (param.type == Token::Type::KeywordForeign) {
            if (m_tokens[m_offset++].type != Token::Type::KeywordKey)
                return Core::DbError { "Expected 'KEY' after 'FOREIGN'", m_offset - 1 };
            if (m_tokens[m_offset++].type != Token::Type::KeywordReferences)
                return Core::DbError { "Expected 'REFERENCES' after 'FOREIGN KEY'", m_offset - 1 };

            auto referenced_table = m_tokens[m_offset++];
            if (referenced_table.type != Token::Type::Identifier) {
                return expected("referenced table name", m_tokens[m_offset - 1], m_offset);
            }

            if (m_tokens[m_offset++].type != Token::Type::ParenOpen) {
                return expected("'('", m_tokens[m_offset - 1], m_offset);
            }

            auto referenced_column = m_tokens[m_offset++];
            if (referenced_column.type != Token::Type::Identifier) {
                return expected("referenced column name", m_tokens[m_offset - 1], m_offset);
            }

            if (m_tokens[m_offset++].type != Token::Type::ParenClose) {
                return expected("')'", m_tokens[m_offset - 1], m_offset);
            }

            key = Core::ForeignKey { .local_column = name.value, .referenced_table = referenced_table.value, .referenced_column = referenced_column.value };
        }
        else
            return Core::DbError { "Invalid param for column: '" + param.value + "'", m_offset };
    }
    return Core::AST::ParsedColumn {
        .column = Core::Column { name.value, *type, auto_increment, unique, not_null, std::move(default_value) },
        .key = std::move(key)
    };
}

Core::DbErrorOr<std::unique_ptr<Core::AST::CreateTable>> Parser::parse_create_table() {
    auto start = m_offset;
    m_offset += 2; // CREATE TABLE

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen)
        return std::make_unique<Core::AST::CreateTable>(start, table_name.value, std::vector<Core::AST::ParsedColumn> {}, std::make_shared<Core::AST::Check>(start));
    m_offset++;

    std::vector<Core::AST::ParsedColumn> columns;

    std::shared_ptr<Core::AST::Check> check = std::make_shared<Core::AST::Check>(start);

    while (true) {
        auto column = TRY(parse_column());
        columns.push_back(std::move(column));

        while (true) {
            auto keyword = m_tokens[m_offset];
            if (keyword.type == Token::Type::KeywordCheck) {
                m_offset++;
                if (check->main_rule())
                    Core::DbError { "Default rule already exists!", m_offset - 1 };

                auto expr = TRY(parse_expression());
                TRY(check->add_check(std::move(expr)));
            }
            else if (keyword.type == Token::Type::KeywordConstraint) {
                m_offset++;

                auto identifier = m_tokens[m_offset];

                if (identifier.type != Token::Type::Identifier)
                    return expected("identifier", identifier, m_offset - 1);
                m_offset++;

                if (check->constraints().find(identifier.value) != check->constraints().end())
                    return Core::DbError { "Constraint with name '" + identifier.value + "' already exists!", m_offset - 1 };

                if (m_tokens[m_offset].type != Token::Type::KeywordCheck)
                    return expected("'CHECK' after identifier", identifier, m_offset - 1);
                m_offset++;

                auto expr = TRY(parse_expression());

                TRY(check->add_constraint(identifier.value, std::move(expr)));
            }
            else
                break;
        }

        if (m_tokens[m_offset].type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto paren_close = m_tokens[m_offset++];
    if (paren_close.type != Token::Type::ParenClose)
        return expected("')' to close column list", paren_close, m_offset - 1);

    return std::make_unique<Core::AST::CreateTable>(start, table_name.value, std::move(columns), std::move(check));
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

    std::vector<Core::AST::ParsedColumn> to_add;
    std::vector<Core::AST::ParsedColumn> to_alter;
    std::vector<std::string> to_drop;
    std::shared_ptr<Core::AST::Expression> check_to_add = nullptr;
    std::shared_ptr<Core::AST::Expression> check_to_alter = nullptr;
    bool check_to_drop = false;
    std::vector<std::pair<std::string, std::shared_ptr<Core::AST::Expression>>> constraint_to_add;
    std::vector<std::pair<std::string, std::shared_ptr<Core::AST::Expression>>> constraint_to_alter;
    std::vector<std::string> constraint_to_drop;

    while (true) {
        if (m_tokens[m_offset].type == Token::Type::KeywordAdd) {
            m_offset++;

            auto thing_to_add = m_tokens[m_offset++];

            if (thing_to_add.type == Token::Type::Identifier) {
                m_offset--;
                to_add.push_back(TRY(parse_column()));
            }
            else if (thing_to_add.type == Token::Type::KeywordCheck) {
                if (check_to_add)
                    Core::DbError { "Check already added!", m_offset };
                check_to_add = TRY(parse_expression());
            }
            else if (thing_to_add.type == Token::Type::KeywordConstraint) {
                auto constraint_token = m_tokens[m_offset++];
                if (constraint_token.type != Token::Type::Identifier)
                    return expected("constraint name", constraint_token, m_offset - 1);

                auto check = m_tokens[m_offset++];
                if (check.type != Token::Type::KeywordCheck)
                    return expected("'CHECK' keyword after '" + constraint_token.value + "'", check, m_offset - 1);

                auto expr = TRY(parse_expression());

                constraint_to_add.push_back(std::make_pair(constraint_token.value, std::move(expr)));
            }
            else {
                return expected("thing to alter", m_tokens[m_offset], m_offset - 1);
            }
        }
        else if (m_tokens[m_offset].type == Token::Type::KeywordAlter) {
            m_offset++;

            auto thing_to_alter = m_tokens[m_offset++];

            if (thing_to_alter.type == Token::Type::KeywordColumn) {
                to_alter.push_back(TRY(parse_column()));
            }
            else if (thing_to_alter.type == Token::Type::KeywordCheck) {
                if (check_to_alter)
                    Core::DbError { "Check already altered!", m_offset };
                check_to_alter = TRY(parse_expression());
            }
            else if (thing_to_alter.type == Token::Type::KeywordConstraint) {
                auto constraint_token = m_tokens[m_offset++];
                if (constraint_token.type != Token::Type::Identifier)
                    return expected("constraint name", constraint_token, m_offset - 1);

                auto check = m_tokens[m_offset++];
                if (check.type != Token::Type::KeywordCheck)
                    return expected("'CHECK' keyword after '" + constraint_token.value + "'", check, m_offset - 1);

                auto expr = TRY(parse_expression());

                constraint_to_alter.push_back(std::make_pair(constraint_token.value, std::move(expr)));
            }
            else {
                return expected("thing to alter", m_tokens[m_offset], m_offset - 1);
            }
        }
        else if (m_tokens[m_offset].type == Token::Type::KeywordDrop) {
            m_offset++;

            auto thing_to_drop = m_tokens[m_offset++];

            if (thing_to_drop.type == Token::Type::KeywordColumn) {
                while (true) {
                    auto column_token = m_tokens[m_offset++];
                    if (column_token.type != Token::Type::Identifier)
                        return expected("column name", column_token, m_offset - 1);

                    to_drop.push_back(column_token.value);

                    auto comma = m_tokens[m_offset];
                    if (comma.type != Token::Type::Comma)
                        break;
                    m_offset++;
                }
            }
            else if (thing_to_drop.type == Token::Type::KeywordCheck) {
                if (check_to_drop)
                    Core::DbError { "Check already dropped!", m_offset };
                check_to_drop = true;
            }
            else if (thing_to_drop.type == Token::Type::KeywordConstraint) {
                auto constraint_token = m_tokens[m_offset++];
                if (constraint_token.type != Token::Type::Identifier)
                    return expected("constraint name", constraint_token, m_offset - 1);

                constraint_to_drop.push_back(constraint_token.value);
            }
            else {
                return expected("thing to drop", m_tokens[m_offset], m_offset - 1);
            }
        }
        else {
            return Core::DbError { "Unrecognized option", m_offset - 1 };
        }
        if (m_tokens[m_offset].type != Token::Type::Comma)
            break;
        m_offset++;
    }

    return std::make_unique<Core::AST::AlterTable>(start, table_name.value,
        std::move(to_add), std::move(to_alter), std::move(to_drop),
        std::move(check_to_add), std::move(check_to_alter), check_to_drop,
        std::move(constraint_to_add), std::move(constraint_to_alter), std::move(constraint_to_drop));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::InsertInto>> Parser::parse_insert_into() {
    auto start = m_offset;
    m_offset += 2; // INSERT INTO

    auto table_name = m_tokens[m_offset++];
    if (table_name.type != Token::Type::Identifier)
        return expected("table name", table_name, m_offset - 1);

    auto paren_open = m_tokens[m_offset];
    if (paren_open.type != Token::Type::ParenOpen && paren_open.type != Token::Type::KeywordValues)
        return std::make_unique<Core::AST::InsertInto>(start, table_name.value, std::vector<std::string> {}, std::vector<std::unique_ptr<Core::AST::Expression>> {});

    std::vector<std::string> columns;
    if (paren_open.type == Token::Type::ParenOpen) {
        m_offset++;
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
    }

    auto value_token = m_tokens[m_offset++];
    if (value_token.type == Token::Type::KeywordValues) {
        std::vector<std::unique_ptr<Core::AST::Expression>> values;

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

        auto paren_close = m_tokens[m_offset++];
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
    else if (token.type == Token::Type::ParenOpen) {
        auto postfix = m_tokens[m_offset + 1];
        if (postfix.type == Token::Type::KeywordSelect) {
            m_offset++;

            lhs = std::make_unique<Core::AST::SelectExpression>(start, TRY(parse_select()));
        }
        else {
            m_offset++;
            lhs = TRY(parse_expression());
        }

        auto paren_close = m_tokens[m_offset++];

        if (paren_close.type != Token::Type::ParenClose)
            expected("')' to close expression", paren_close, m_offset - 1);
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

Core::DbErrorOr<std::unique_ptr<Core::AST::TableExpression>> Parser::parse_table_expression() {
    std::unique_ptr<Core::AST::TableExpression> lhs;

    auto start = m_offset;
    auto token = m_tokens[m_offset];

    // std::cout << "parse_expression " << token.value << std::endl;
    if (token.type == Token::Type::Identifier) {
        lhs = TRY(parse_table_identifier());
    }
    else if (token.type == Token::Type::ParenOpen) {
        auto postfix = m_tokens[m_offset + 1];
        if (postfix.type == Token::Type::KeywordSelect) {
            m_offset++;
            lhs = std::make_unique<Core::AST::SelectTableExpression>(start, TRY(parse_select()));
        }
        else {
            m_offset++;
            lhs = TRY(parse_table_expression());
        }

        auto paren_close = m_tokens[m_offset++];

        if (paren_close.type != Token::Type::ParenClose)
            expected("')' to close expression", paren_close, m_offset - 1);
    }
    else {
        return expected("table or expression", token, start);
    }

    auto maybe_operator = TRY(parse_join_expression(std::move(lhs)));
    assert(maybe_operator);
    return maybe_operator;
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> Parser::parse_expression_or_index(Core::AST::SelectColumns const& select_columns) {
    auto token = m_tokens[m_offset];
    if (token.type == Token::Type::Int) {
        m_offset++;
        auto index = std::stoi(token.value);
        if (select_columns.select_all()) {
            return Core::DbError { "Index is not allowed when using SELECT *", m_offset - 1 };
        }
        if (index < 1) {
            return Core::DbError { "Index must be positive, " + token.value + " given", m_offset - 1 };
        }
        if (static_cast<size_t>(index) > select_columns.columns().size()) {
            return Core::DbError { "Index is out of range", m_offset - 1 };
        }
        return std::make_unique<Core::AST::NonOwningExpressionProxy>(m_offset - 1, *select_columns.columns()[index - 1].column);
    }
    return parse_expression();
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
        return std::make_unique<Core::AST::Literal>(start, Core::Value::create_time(token.value, Util::SimulationClock::Format::NO_CLOCK_AMERICAN));
    }
    else if (token.type == Token::Type::KeywordNull) {
        m_offset++;
        return std::make_unique<Core::AST::Literal>(start, Core::Value::null());
    }

    return expected("literal", m_tokens[m_offset], m_offset - 1);
}

static int operator_precedence(Token::Type op) {
    switch (op) {
    case Token::Type::KeywordIs:
    case Token::Type::KeywordLike:
    case Token::Type::KeywordMatch:
    case Token::Type::OpEqual:
    case Token::Type::OpNotEqual:
    case Token::Type::OpGreater:
    case Token::Type::OpLess:
        return 500;
    case Token::Type::KeywordBetween:
    case Token::Type::KeywordIn:
        return 200;
    case Token::Type::KeywordAnd:
        return 150;
    case Token::Type::KeywordOr:
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

Core::DbErrorOr<Parser::BetweenRange> Parser::parse_between_range() {
    auto min = TRY(parse_expression(operator_precedence(Token::Type::KeywordBetween) + 1));

    if (m_tokens[m_offset++].type != Token::Type::KeywordAnd)
        return expected("'AND' in 'BETWEEN'", m_tokens[m_offset], m_offset - 1);

    auto max = TRY(parse_expression(operator_precedence(Token::Type::KeywordBetween) + 1));

    return Parser::BetweenRange { std::move(min), std::move(max) };
}

static bool is_binary_operator(Token::Type op) {
    switch (op) {
    case Token::Type::KeywordAnd:
    case Token::Type::KeywordOr:
    case Token::Type::KeywordBetween:
    case Token::Type::KeywordIn:
    case Token::Type::KeywordIs:
    case Token::Type::KeywordLike:
    case Token::Type::KeywordMatch:
    case Token::Type::OpEqual:
    case Token::Type::OpNotEqual:
    case Token::Type::OpGreater:
    case Token::Type::OpLess:
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

static bool is_join_expression(Token const& token) {
    if (token.type == Token::Type::Identifier) {
        if (Parser::compare_case_insensitive(token.value, "LEFT") || Parser::compare_case_insensitive(token.value, "RIGHT"))
            return true;
    }
    switch (token.type) {
    case Token::Type::KeywordInner:
    case Token::Type::KeywordOuter:
    case Token::Type::KeywordFull:
    case Token::Type::Comma:
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
    case Token::Type::KeywordLike:
        return Core::AST::BinaryOperator::Operation::Like;
        break;
    case Token::Type::KeywordMatch:
        return Core::AST::BinaryOperator::Operation::Match;
        break;
    case Token::Type::KeywordAnd:
        return Core::AST::BinaryOperator::Operation::And;
        break;
    case Token::Type::KeywordOr:
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

static Core::AST::JoinExpression::Type token_to_join_operation(Token token) {
    if (token.type == Token::Type::Identifier) {
        if (Parser::compare_case_insensitive(token.value, "LEFT"))
            return Core::AST::JoinExpression::Type::LeftJoin;
        if (Parser::compare_case_insensitive(token.value, "RIGHT"))
            return Core::AST::JoinExpression::Type::RightJoin;
    }
    switch (token.type) {
    case Token::Type::KeywordInner:
        return Core::AST::JoinExpression::Type::InnerJoin;
        break;
    case Token::Type::KeywordOuter:
        return Core::AST::JoinExpression::Type::OuterJoin;
        break;
    default:
        return Core::AST::JoinExpression::Type::Invalid;
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

        using Variant = std::variant<BetweenRange, InArgs, IsArgs, std::unique_ptr<Core::AST::Expression>>;

        Variant rhs = TRY([&]() -> Core::DbErrorOr<Variant> {
            if (current_operator == Token::Type::KeywordBetween)
                return TRY(parse_between_range());
            if (current_operator == Token::Type::KeywordIn)
                return TRY(parse_in());
            if (current_operator == Token::Type::KeywordIs)
                return TRY(parse_is());
            return TRY(parse_expression(current_precedence));
        }());
        // std::cout << "3. " << m_offset << ": " << m_tokens[m_offset].value << std::endl;

        auto next_operator = peek_operator();

        auto next_precedence = operator_precedence(next_operator);

        if (current_precedence > next_precedence) {
            if (current_operator == Token::Type::KeywordBetween) {
                auto rhs_between_range = std::move(std::get<BetweenRange>(rhs));
                lhs = std::make_unique<Core::AST::BetweenExpression>(std::move(lhs), std::move(rhs_between_range.min), std::move(rhs_between_range.max));
            }
            else if (current_operator == Token::Type::KeywordIn) {
                auto rhs_in_args = std::move(std::get<InArgs>(rhs));
                lhs = std::make_unique<Core::AST::InExpression>(std::move(lhs), std::move(rhs_in_args.args));
            }
            else if (current_operator == Token::Type::KeywordIs) {
                auto rhs_is_args = std::move(std::get<IsArgs>(rhs));
                lhs = std::make_unique<Core::AST::IsExpression>(std::move(lhs), rhs_is_args.what);
            }
            else if (is_binary_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), token_type_to_binary_operation(current_operator),
                    std::move(std::get<std::unique_ptr<Core::AST::Expression>>(rhs)));
            }
            else if (is_arithmetic_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::ArithmeticOperator>(std::move(lhs), token_type_to_arithmetic_operation(current_operator),
                    std::move(std::get<std::unique_ptr<Core::AST::Expression>>(rhs)));
            }
        }
        else {
            if (current_operator == Token::Type::KeywordBetween) {
                auto rhs_between_range = std::move(std::get<BetweenRange>(rhs));
                lhs = std::make_unique<Core::AST::BetweenExpression>(std::move(lhs), std::move(rhs_between_range.min), TRY(parse_operand(std::move(rhs_between_range.max))));
            }
            else if (current_operator == Token::Type::KeywordIn) {
                auto rhs_in_args = std::move(std::get<InArgs>(rhs));
                lhs = std::make_unique<Core::AST::InExpression>(std::move(lhs), std::move(rhs_in_args.args));
            }
            else if (current_operator == Token::Type::KeywordIs) {
                auto rhs_is_args = std::move(std::get<IsArgs>(rhs));
                lhs = std::make_unique<Core::AST::IsExpression>(std::move(lhs), rhs_is_args.what);
            }
            else if (is_binary_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::BinaryOperator>(std::move(lhs), token_type_to_binary_operation(current_operator),
                    TRY(parse_operand(std::move(std::get<std::unique_ptr<Core::AST::Expression>>(rhs)))));
            }
            else if (is_arithmetic_operator(current_operator)) {
                lhs = std::make_unique<Core::AST::ArithmeticOperator>(std::move(lhs), token_type_to_arithmetic_operation(current_operator),
                    TRY(parse_operand(std::move(std::get<std::unique_ptr<Core::AST::Expression>>(rhs)))));
            }
        }
    }
}

Core::DbErrorOr<std::unique_ptr<Core::AST::TableExpression>> Parser::parse_join_expression(std::unique_ptr<Core::AST::TableExpression> lhs) {
    while (true) {
        auto current = m_tokens[m_offset];
        if (!is_join_expression(current))
            return lhs;
        m_offset++;

        if (current.type == Token::Type::Comma) {
            auto rhs = TRY(parse_table_expression());
            lhs = std::make_unique<Core::AST::CrossJoinExpression>(m_offset, std::move(lhs), std::move(rhs));
        }
        else {
            if (current.type == Token::Type::KeywordFull) {
                current = m_tokens[m_offset];

                if (current.type != Token::Type::KeywordOuter) {
                    return expected("'OUTER' after 'FULL'", m_tokens[m_offset], m_offset);
                }

                m_offset++;
            }

            if (m_tokens[m_offset++].type != Token::Type::KeywordJoin) {
                return expected("'JOIN' after " + m_tokens[m_offset - 2].value, m_tokens[m_offset - 1], m_offset);
            }

            auto rhs = TRY(parse_table_expression());

            if (m_tokens[m_offset++].type != Token::Type::KeywordOn) {
                return expected("'ON' expression after 'JOIN'", m_tokens[m_offset - 1], m_offset);
            }

            auto on_lhs = TRY(parse_identifier());

            if (m_tokens[m_offset++].type != Token::Type::OpEqual) {
                return expected("'=' after column name", m_tokens[m_offset - 1], m_offset);
            }

            auto on_rhs = TRY(parse_identifier());
            lhs = std::make_unique<Core::AST::JoinExpression>(m_offset, std::move(lhs), std::move(on_lhs), token_to_join_operation(current), std::move(rhs), std::move(on_rhs));
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
            auto column = TRY(parse_identifier());

            if (m_tokens[m_offset++].type != Token::Type::ParenClose)
                return expected("')' to close aggregate function", m_tokens[m_offset], m_offset - 1);

            std::optional<std::string> over;

            if (m_tokens[m_offset].type == Token::Type::KeywordOver) {
                m_offset++;
                if (m_tokens[m_offset++].type != Token::Type::ParenOpen)
                    return expected("'(' for 'OVER PARTITION' clause", m_tokens[m_offset], m_offset - 1);

                if (m_tokens[m_offset++].type != Token::Type::KeywordPartition)
                    return expected("'PARTITION' for 'OVER PARTITION' clause", m_tokens[m_offset], m_offset - 1);

                if (m_tokens[m_offset++].type != Token::Type::KeywordBy)
                    return expected("'BY' after 'PARTITION'", m_tokens[m_offset], m_offset - 1);

                auto identifier = m_tokens[m_offset++];

                if (identifier.type != Token::Type::Identifier)
                    return expected("identifier after 'PARTITION BY'", m_tokens[m_offset], m_offset - 1);

                over = identifier.value;

                if (m_tokens[m_offset++].type != Token::Type::ParenClose)
                    return expected("')' to close 'OVER' clause", m_tokens[m_offset], m_offset - 1);
            }

            return { std::make_unique<Core::AST::AggregateFunction>(m_offset, aggregate_function, std::move(column), std::move(over)) };
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

Core::DbErrorOr<Parser::InArgs> Parser::parse_in() {
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
    return { std::move(args) };
}

Core::DbErrorOr<Parser::IsArgs> Parser::parse_is() {
    auto token = m_tokens[m_offset++];
    if (token.type == Token::Type::KeywordNull) {
        return Parser::IsArgs { Core::AST::IsExpression::What::Null };
    }
    if (token.type == Token::Type::KeywordNot) {
        token = m_tokens[m_offset++];
        if (token.type == Token::Type::KeywordNull)
            return Parser::IsArgs { Core::AST::IsExpression::What::NotNull };
        return expected("'NULL' after 'IS NOT'", token, m_offset - 1);
    }
    return expected("'NULL' or 'NOT NULL' after 'IS'", token, m_offset - 1);
}

Core::DbErrorOr<std::unique_ptr<Core::AST::Identifier>> Parser::parse_identifier() {
    auto name = m_tokens[m_offset++];
    std::optional<std::string> table = {};

    if (name.type != Token::Type::Identifier)
        return expected("identifier", name, m_offset - 1);

    if (m_tokens[m_offset].type == Token::Type::Period) {
        m_offset++;

        if (!table)
            table = name.value;

        name = m_tokens[m_offset++];

        if (name.type != Token::Type::Identifier)
            return expected("identifier", name, m_offset - 1);
    }

    return std::make_unique<Core::AST::Identifier>(m_offset - 1, name.value, std::move(table));
}

Core::DbErrorOr<std::unique_ptr<Core::AST::TableIdentifier>> Parser::parse_table_identifier() {
    auto name = m_tokens[m_offset++];
    std::optional<std::string> alias = {};

    if (name.type != Token::Type::Identifier)
        return expected("identifier", name, m_offset - 1);

    auto alias_token = m_tokens[m_offset];
    // Don't allow LEFT/RIGHT as aliases because they are used for joins
    if (alias_token.type == Token::Type::Identifier && alias_token.value != "LEFT" && alias_token.value != "RIGHT") {
        m_offset++;
        alias = alias_token.value;
    }
    else if (alias_token.type == Token::Type::KeywordAs) {
        m_offset++;

        alias_token = m_tokens[m_offset++];

        if (alias_token.type != Token::Type::Identifier)
            return expected("identifier", alias_token, m_offset - 1);
        alias = alias_token.value;
    }
    return std::make_unique<Core::AST::TableIdentifier>(m_offset - 1, name.value, alias);
}

Core::DbError Parser::expected(std::string what, Token got, size_t offset) {
    return Core::DbError { "Expected " + what + ", got '" + got.value + "'", offset };
}

}
