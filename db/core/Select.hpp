#pragma once

#include "Expression.hpp"
#include "AST.hpp"
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
    enum class GroupOrPartition{
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

class Select : public Statement, public Expression {
public:
    struct SelectOptions {
        SelectColumns columns;
        std::optional<std::string> from;
        std::unique_ptr<Expression> where = {};
        std::optional<OrderBy> order_by = {};
        std::optional<Top> top = {};
        std::optional<GroupBy> group_by = {};
        std::unique_ptr<Expression> having = {};
        bool distinct = false;
        std::optional<std::string> select_into = {};
    };

    Select(ssize_t start, SelectOptions options)
        : ASTNode(start), Statement(start), Expression(start)
        , m_options(std::move(options)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;
    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override;
    virtual std::string to_string() const override{return "SELECT(TODO)";}

private:
    DbErrorOr<std::vector<TupleWithSource>> collect_rows(EvaluationContext&, AbstractTable&) const;

    SelectOptions m_options;
};

class Union : public Statement {
public:
    Union(ssize_t start, std::unique_ptr<Select> lhs, std::unique_ptr<Select> rhs, bool distinct)
        : ASTNode(start), Statement(start)
        , m_lhs(std::move(lhs))
        , m_rhs(std::move(rhs))
        , m_distinct(distinct) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::unique_ptr<Select> m_lhs;
    std::unique_ptr<Select> m_rhs;
    bool m_distinct;
};

}
