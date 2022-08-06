#pragma once

#include "AST.hpp"
#include "Expression.hpp"
#include "db/core/Database.hpp"
#include <memory>
#include <optional>
#include <string>

namespace Db::Core::AST {

struct OrderBy {
    enum class Order {
        Ascending,
        Descending
    };

    struct OrderBySet {
        ExpressionOrIndex column;
        Order order = Order::Ascending;
    };

    std::vector<OrderBySet> columns;
};

struct GroupBy {
    enum class GroupOrPartition {
        GROUP,
        PARTITION
    };
    GroupOrPartition type;

    std::vector<std::string> columns;

    bool is_valid(std::string const& rhs) const {
        bool result = 0;
        for (const auto& lhs : columns) {
            result |= (lhs == rhs);
        }

        return result;
    }
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
    struct SelectOptions {
        SelectColumns columns;
        std::unique_ptr<TableExpression> from;
        std::unique_ptr<Expression> where = {};
        std::optional<OrderBy> order_by = {};
        std::optional<Top> top = {};
        std::optional<GroupBy> group_by = {};
        std::unique_ptr<Expression> having = {};
        bool distinct = false;
        std::optional<std::string> select_into = {};
    };

    Select(size_t start, SelectOptions options)
        : m_start(start)
        , m_options(std::move(options)) { }

    DbErrorOr<ResultSet> execute(Database&) const;

private:
    DbErrorOr<std::vector<TupleWithSource>> collect_rows(EvaluationContext&, AbstractTable&) const;

    size_t m_start {};
    SelectOptions m_options;
};

class SelectExpression : public Expression {
public:
    SelectExpression(ssize_t start, Select select)
        : Expression(start)
        , m_select(std::move(select)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override;
    virtual std::string to_string() const override { return "(SELECT TODO)"; }

private:
    Select m_select;
};

class SelectTableExpression : public TableExpression {
public:
    SelectTableExpression(ssize_t start, Select select)
        : TableExpression(start)
        , m_select(std::move(select)) { }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(Database* db) const override;
    virtual std::string to_string() const override { return "(SELECT TODO)"; }

private:
    Select m_select;
};

class SelectStatement : public Statement {
public:
    SelectStatement(ssize_t start, Select select)
        : Statement(start)
        , m_select(std::move(select)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    Select m_select;
};

class Union : public Statement {
public:
    Union(ssize_t start, Select lhs, Select rhs, bool distinct)
        : Statement(start)
        , m_lhs(std::move(lhs))
        , m_rhs(std::move(rhs))
        , m_distinct(distinct) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    Select m_lhs;
    Select m_rhs;
    bool m_distinct;
};

class InsertInto : public Statement {
public:
    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, std::vector<std::unique_ptr<Core::AST::Expression>> values)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_values(std::move(values)) { }

    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, Core::AST::Select select)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_select(std::move(select)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
    std::vector<std::string> m_columns;
    std::vector<std::unique_ptr<Core::AST::Expression>> m_values;
    std::optional<Core::AST::Select> m_select;
};

}
