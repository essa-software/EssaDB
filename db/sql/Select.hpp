#pragma once

#include <db/core/Database.hpp>
#include <db/sql/ast/Expression.hpp>
#include <db/sql/ast/TableExpression.hpp>
#include <memory>
#include <optional>
#include <string>

namespace Db::Sql::AST {

struct OrderBy {
    enum class Order {
        Ascending,
        Descending
    };

    struct OrderBySet {
        std::unique_ptr<Expression> expression;
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

    Core::DbErrorOr<Core::ResultSet> execute(EvaluationContext&) const;

    auto const& from() const { return m_options.from; }

private:
    Core::DbErrorOr<std::vector<Core::TupleWithSource>> collect_rows(EvaluationContext&, Core::Relation&) const;

    size_t m_start {};
    SelectOptions m_options;
};

}
