#pragma once

#include "Row.hpp"
#include "SelectResult.hpp"
#include "Table.hpp"
#include "Value.hpp"

#include <memory>
#include <pthread.h>
#include <set>
#include <string>

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

class Identifier : public Expression {
public:
    explicit Identifier(std::string id)
        : m_id(std::move(id)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const override;
    virtual std::string to_string() const override { return m_id; }

private:
    std::string m_id;
};

struct Filter {
    std::string column;
    enum class Operation {
        Equal,        // =
        NotEqual,     // !=
        Greater,      // >
        GreaterEqual, // >=
        Less,         // <
        LessEqual,    // <=
    };
    Operation operation;
    Value rhs;

    DbErrorOr<bool> is_true(Value const& lhs) const {
        switch (operation) {
        case Operation::Equal:
            return TRY(lhs.to_string()) == TRY(rhs.to_string());
        case Operation::NotEqual:
            return TRY(lhs.to_string()) != TRY(rhs.to_string());
        case Operation::Greater:
            return TRY(lhs.to_string()) > TRY(rhs.to_string());
        case Operation::GreaterEqual:
            return TRY(lhs.to_string()) >= TRY(rhs.to_string());
        case Operation::Less:
            return TRY(lhs.to_string()) < TRY(rhs.to_string());
        case Operation::LessEqual:
            return TRY(lhs.to_string()) <= TRY(rhs.to_string());
        }
        __builtin_unreachable();
    }
};

class SelectColumns {
public:
    SelectColumns() = default;
    explicit SelectColumns(std::vector<std::unique_ptr<Expression>> columns)
        : m_columns(std::move(columns)) { }

    // FIXME: This is temporary until we can represent all handled
    //        expressions in SQL.
    SelectColumns(std::vector<std::string> columns) {
        for (auto const& column : columns) {
            m_columns.push_back(std::make_unique<Identifier>(column));
        }
    }

    bool select_all() const { return m_columns.empty(); }
    std::vector<std::unique_ptr<Expression>> const& columns() const { return m_columns; }

private:
    std::vector<std::unique_ptr<Expression>> m_columns {};
};

struct OrderBy {
    enum class Order {
        Ascending,
        Descending
    };

    struct OrderBySet{
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

class Select {
public:
    Select(SelectColumns columns, std::string from, std::optional<Filter> where = {}, std::optional<OrderBy> order_by = {}, std::optional<Top> top = {})
        : m_columns(std::move(columns))
        , m_from(std::move(from))
        , m_where(std::move(where))
        , m_order_by(std::move(order_by))
        , m_top(std::move(top)) { }

    DbErrorOr<Value> execute(Database&) const;

private:
    SelectColumns m_columns;
    std::string m_from;
    std::optional<Filter> m_where;
    std::optional<OrderBy> m_order_by;
    std::optional<Top> m_top;
};

}
