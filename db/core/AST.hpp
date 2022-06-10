#pragma once

#include "Row.hpp"
#include "SelectResult.hpp"
#include "Table.hpp"
#include "Value.hpp"

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

class Identifier : public Expression {
public:
    explicit Identifier(std::string id)
        : m_id(std::move(id)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const override;
    virtual std::string to_string() const override { return m_id; }

private:
    std::string m_id;
};

class Function : public Expression {
public:
    explicit Function(std::string name, std::vector<std::unique_ptr<Expression>> args)
        : m_name(std::move(name))
        , m_args(std::move(args)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const override;
    virtual std::string to_string() const override { return m_name + "(TODO)"; }

private:
    std::string m_name;
    std::vector<std::unique_ptr<Expression>> m_args;
};

struct Filter {
    enum class Operation {
        Equal,        // =
        NotEqual,     // !=
        Greater,      // >
        GreaterEqual, // >=
        Less,         // <
        LessEqual,    // <=
        Like,
        Between,
        In
    };

    enum class LogicOperator{
        AND, OR
    };

    struct FilterSet{
        std::string column;
        Operation operation = Operation::Equal;
        std::vector<DbErrorOr<Value>> args;

        LogicOperator logic = LogicOperator::AND;
    };
    
    std::vector<FilterSet> filter_rules;

    DbErrorOr<bool> is_true(FilterSet const& rule, Value const& lhs) const {
    switch (rule.operation) {
        case Operation::Equal:
            return TRY(lhs.to_string()) == TRY(rule.args[0].value().to_string());
        case Operation::NotEqual:
            return TRY(lhs.to_string()) != TRY(rule.args[0].value().to_string());
        case Operation::Greater:
            return TRY(lhs.to_string()) > TRY(rule.args[0].value().to_string());
        case Operation::GreaterEqual:
            return TRY(lhs.to_string()) >= TRY(rule.args[0].value().to_string());
        case Operation::Less:
            return TRY(lhs.to_string()) < TRY(rule.args[0].value().to_string());
        case Operation::LessEqual:
            return TRY(lhs.to_string()) <= TRY(rule.args[0].value().to_string());
        case Operation::Like:
            return true;
        case Operation::Between:
            return TRY(lhs.to_string()) >= TRY(rule.args[0].value().to_string()) && TRY(lhs.to_string()) <= TRY(rule.args[1].value().to_string());
        case Operation::In:
            for(const auto& arg : rule.args){
                if(TRY(lhs.to_string()) == TRY(arg.value().to_string()))
                    return true;
            }
            return false;
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
    Select(SelectColumns columns, std::string from, std::optional<Filter> where = {}, std::optional<OrderBy> order_by = {}, std::optional<Top> top = {})
        : m_columns(std::move(columns))
        , m_from(std::move(from))
        , m_where(std::move(where))
        , m_order_by(std::move(order_by))
        , m_top(std::move(top)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    SelectColumns m_columns;
    std::string m_from;
    std::optional<Filter> m_where;
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
