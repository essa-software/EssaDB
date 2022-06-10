#pragma once

#include "Row.hpp"
#include "SelectResult.hpp"
#include "Table.hpp"
#include "Value.hpp"
#include "db/core/Column.hpp"
#include "db/core/DbError.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <pthread.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Db::Core {
class Database;
}

namespace Db::Core::AST {

struct EvaluationContext {
    Table const& table;
};

class Expression {
public:
    virtual ~Expression() = default;
    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const = 0;
    virtual std::string to_string() const = 0;
};

class Literal : public Expression {
public:
    explicit Literal(Value val)
        : m_value(std::move(val)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const override { return m_value; }
    virtual std::string to_string() const override { return m_value.to_string().release_value_but_fixme_should_propagate_errors(); }

private:
    Value m_value;
};

class Identifier : public Expression {
public:
    explicit Identifier(std::string id)
        : m_id(std::move(id)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const override;
    virtual std::string to_string() const override { return m_id; }

private:
    std::string m_id;
};

class BinaryOperator : public Expression {
public:
    enum class Operation {
        Equal,        // =
        NotEqual,     // !=
        Greater,      // >
        GreaterEqual, // >=
        Less,         // <
        LessEqual,    // <=
        Like,
        And,
        Or,
        Not,
    };

    BinaryOperator(std::unique_ptr<Expression> lhs, Operation op, std::unique_ptr<Expression> rhs = nullptr)
        : m_lhs(std::move(lhs))
        , m_operation(op)
        , m_rhs(std::move(rhs)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, Row const& row) const override {
        return Value::create_bool(TRY(is_true(context, row)));
    }
    virtual std::string to_string() const override { return "BinaryOperator(TODO)"; }

private:
    DbErrorOr<bool> is_true(EvaluationContext&, Row const&) const;

    std::unique_ptr<Expression> m_lhs;
    Operation m_operation {};
    std::unique_ptr<Expression> m_rhs;
};

class SelectColumns {
public:
    SelectColumns() = default;

    struct Column {
        std::unique_ptr<Expression> column;
        std::optional<std::string> alias = {};
    };

    explicit SelectColumns(std::vector<Column> columns)
        : m_columns(std::move(columns)) { }

    // FIXME: This is temporary until we can represent all handled
    //        expressions in SQL.

    struct IdentifierColumn {
        std::string column;
        std::optional<std::string> alias = {};
    };

    SelectColumns(std::vector<IdentifierColumn> columns) {
        for (auto const& column : columns) {
            m_columns.push_back(Column { .column = std::make_unique<Identifier>(column.column), .alias = column.alias });
            m_aliases.push_back(std::move(column.alias));
        }
    }

    bool select_all() const { return m_columns.empty(); }
    std::vector<Column> const& columns() const { return m_columns; }
    std::vector<std::optional<std::string>> const& aliases() const { return m_aliases; }

private:
    std::vector<Column> m_columns {};
    std::vector<std::optional<std::string>> m_aliases {};
};

struct OrderBy {
    enum class Order {
        Ascending,
        Descending
    };

    struct OrderBySet {
        std::string name;
        Order order = Order::Ascending;
    };

    std::vector<OrderBySet> columns;
};

struct Top {
    enum class Unit {
        Val,
        Perc
    };
    Unit unit = Unit::Perc;
    unsigned value = 100;
};

class Statement {
public:
    virtual ~Statement() = default;
    virtual DbErrorOr<Value> execute(Database&) const = 0;
};

class Select : public Statement {
public:
    Select(SelectColumns columns, std::string from, std::unique_ptr<Expression> where = {}, std::optional<OrderBy> order_by = {}, std::optional<Top> top = {})
        : m_columns(std::move(columns))
        , m_from(std::move(from))
        , m_where(std::move(where))
        , m_order_by(std::move(order_by))
        , m_top(std::move(top)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    SelectColumns m_columns;
    std::string m_from;
    std::unique_ptr<Expression> m_where;
    std::optional<OrderBy> m_order_by;
    std::optional<Top> m_top;
};

class CreateTable : public Statement {
public:
    CreateTable(std::string name, std::vector<Column> columns)
        : m_name(std::move(name))
        , m_columns(std::move(columns)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
    std::vector<Column> m_columns;
};

}
