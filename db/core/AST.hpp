#pragma once

#include "SelectResult.hpp"
#include "Table.hpp"
#include "Tuple.hpp"
#include "Value.hpp"
#include "db/core/Column.hpp"
#include "db/core/DbError.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <pthread.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

namespace Db::Core {
class Database;
}

namespace Db::Core::AST {

struct EvaluationContext;

class ASTNode {
public:
    explicit ASTNode(ssize_t start)
        : m_start(start) { }

    auto start() const { return m_start; }

private:
    size_t m_start;
};

class Expression : public ASTNode {
public:
    explicit Expression(ssize_t start)
        : ASTNode(start) { }

    virtual ~Expression() = default;
    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Tuple const&) const = 0;
    virtual std::string to_string() const = 0;
    virtual std::vector<std::string> referenced_columns() const { return {}; }
};

class Literal : public Expression {
public:
    explicit Literal(ssize_t start, Value val)
        : Expression(start)
        , m_value(std::move(val)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Tuple const&) const override { return m_value; }
    virtual std::string to_string() const override { return m_value.to_string().release_value_but_fixme_should_propagate_errors(); }

private:
    Value m_value;
};

class Identifier : public Expression {
public:
    explicit Identifier(ssize_t start, std::string id)
        : Expression(start)
        , m_id(std::move(id)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Tuple const&) const override;
    virtual std::string to_string() const override { return m_id; }
    virtual std::vector<std::string> referenced_columns() const override { return { m_id }; }

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
        Not, // FIXME: This is not binary op

        Invalid
    };

    BinaryOperator(std::unique_ptr<Expression> lhs, Operation op, std::unique_ptr<Expression> rhs = nullptr)
        : Expression(lhs->start())
        , m_lhs(std::move(lhs))
        , m_operation(op)
        , m_rhs(std::move(rhs)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, Tuple const& row) const override {
        return Value::create_bool(TRY(is_true(context, row)));
    }
    virtual std::string to_string() const override { return "BinaryOperator(" + m_lhs->to_string() + "," + m_rhs->to_string() + ")"; }

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        auto rhs_columns = m_rhs->referenced_columns();
        lhs_columns.insert(lhs_columns.begin(), rhs_columns.begin(), rhs_columns.end());
        return lhs_columns;
    }

private:
    DbErrorOr<bool> is_true(EvaluationContext&, Tuple const&) const;

    std::unique_ptr<Expression> m_lhs;
    Operation m_operation {};
    std::unique_ptr<Expression> m_rhs;
};

class ArithmeticOperator : public Expression {
public:
    enum class Operation {
        Add,
        Sub,
        Mul,
        Div,

        Invalid
    };

    ArithmeticOperator(std::unique_ptr<Expression> lhs, Operation op, std::unique_ptr<Expression> rhs = nullptr)
        : Expression(lhs->start())
        , m_lhs(std::move(lhs))
        , m_operation(op)
        , m_rhs(std::move(rhs)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, Tuple const& row) const override;

    virtual std::string to_string() const override { return "BinaryOperator(" + m_lhs->to_string() + "," + m_rhs->to_string() + ")"; }

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        auto rhs_columns = m_rhs->referenced_columns();
        lhs_columns.insert(lhs_columns.end(), rhs_columns.begin(), rhs_columns.end());
        return lhs_columns;
    }

private:
    DbErrorOr<bool> is_true(EvaluationContext&, Tuple const&) const;

    std::unique_ptr<Expression> m_lhs;
    Operation m_operation {};
    std::unique_ptr<Expression> m_rhs;
};

class BetweenExpression : public Expression {
public:
    BetweenExpression(std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> min, std::unique_ptr<Expression> max)
        : Expression(lhs->start())
        , m_lhs(std::move(lhs))
        , m_min(std::move(min))
        , m_max(std::move(max)) {
        assert(m_lhs);
        assert(m_min);
        assert(m_max);
    }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, Tuple const& row) const override;
    virtual std::string to_string() const override {
        return "BetweenExpression(" + m_lhs->to_string() + "," + m_min->to_string() + "," + m_max->to_string() + ")";
    }

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        auto min_columns = m_min->referenced_columns();
        auto max_columns = m_max->referenced_columns();
        lhs_columns.insert(lhs_columns.end(), min_columns.begin(), min_columns.end());
        lhs_columns.insert(lhs_columns.end(), max_columns.begin(), max_columns.end());
        return lhs_columns;
    }

private:
    std::unique_ptr<Expression> m_lhs;
    std::unique_ptr<Expression> m_min;
    std::unique_ptr<Expression> m_max;
};

class InExpression : public Expression {
public:
    InExpression(std::unique_ptr<Expression> lhs, std::vector<std::unique_ptr<Core::AST::Expression>> args)
        : Expression(lhs->start())
        , m_lhs(std::move(lhs))
        , m_args(std::move(args)) {
        assert(m_lhs);
    }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, Tuple const& row) const override;
    virtual std::string to_string() const override {
        std::string result = "InExpression(";
        for (auto i = m_args.begin(); i < m_args.end(); i++) {
            result += (*i)->to_string();
            if (i != m_args.end() - 1)
                result += ", ";
        }

        result += ")";

        return result;
    }

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        for (auto const& arg : m_args) {
            auto arg_columns = arg->referenced_columns();
            lhs_columns.insert(lhs_columns.end(), arg_columns.begin(), arg_columns.end());
        }
        return lhs_columns;
    }

private:
    std::unique_ptr<Expression> m_lhs;
    std::vector<std::unique_ptr<Core::AST::Expression>> m_args;
};

class CaseExpression : public Expression {
public:
    struct CasePair {
        std::unique_ptr<Expression> expr;
        std::unique_ptr<Expression> value;
    };

    CaseExpression(std::vector<CasePair> cases, std::unique_ptr<Core::AST::Expression> else_value = {})
        : Expression(cases.front().expr->start())
        , m_cases(std::move(cases))
        , m_else_value(std::move(else_value)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, Tuple const& row) const override;
    virtual std::string to_string() const override {
        std::string result = "CaseExpression: \n";
        for (const auto& c : m_cases) {
            result += "if expression then " + c.value->to_string() + "\n";
        }

        if (m_else_value)
            result += "else " + m_else_value->to_string();

        return result;
    }

    virtual std::vector<std::string> referenced_columns() const override {
        auto else_columns = m_else_value->referenced_columns();
        for (auto const& case_ : m_cases) {
            auto value_columns = case_.value->referenced_columns();
            else_columns.insert(else_columns.end(), value_columns.begin(), value_columns.end());
            auto expr_columns = case_.expr->referenced_columns();
            else_columns.insert(else_columns.end(), expr_columns.begin(), expr_columns.end());
        }
        return else_columns;
    }

private:
    std::vector<CasePair> m_cases;

    std::unique_ptr<Core::AST::Expression> m_else_value;
};

class SelectColumns {
public:
    SelectColumns() = default;

    struct Column {
        std::optional<std::string> alias = {};
        std::unique_ptr<Expression> column;
    };

    explicit SelectColumns(std::vector<Column> columns);

    bool select_all() const { return m_columns.empty(); }
    std::vector<Column> const& columns() const { return m_columns; }

    struct ResolvedAlias {
        Expression& column;
        size_t index;
    };

    DbErrorOr<ResolvedAlias> resolve_alias(std::string const& alias) const;

private:
    std::vector<Column> m_columns;
    std::map<std::string, ResolvedAlias> m_aliases;
};

struct EvaluationContext {
    SelectColumns const& columns;
    Table const* table = nullptr;
    std::vector<Tuple> const* row_group = nullptr;
    enum class RowType {
        FromTable,
        FromResultSet
    };
    RowType row_type;
};

class ExpressionOrIndex : public std::variant<size_t, std::unique_ptr<Expression>> {
public:
    using Variant = std::variant<size_t, std::unique_ptr<Expression>>;

    explicit ExpressionOrIndex(size_t index)
    : Variant{index} {}

    explicit ExpressionOrIndex(std::unique_ptr<Expression> expr)
    : Variant(std::move(expr)) {}

    bool is_index() const { return std::holds_alternative<size_t>(*this); }
    size_t index() const { return std::get<size_t>(*this); }

    bool is_expression() const { return std::holds_alternative<std::unique_ptr<Expression>>(*this); }
    Expression& expression() const { return *std::get<std::unique_ptr<Expression>>(*this); }

    DbErrorOr<Value> evaluate(EvaluationContext& context, Tuple const& input) const;
};

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

class Statement : public ASTNode {
public:
    explicit Statement(ssize_t start)
        : ASTNode(start) { }

    virtual ~Statement() = default;
    virtual DbErrorOr<Value> execute(Database&) const = 0;
};

class Select : public Statement {
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
        : Statement(start)
        , m_options(std::move(options)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    DbErrorOr<std::vector<Tuple>> collect_rows(EvaluationContext& context, Table const& table, std::vector<Tuple> const& input_rows) const;

    SelectOptions m_options;
};

class Union : public Statement {
public:
    Union(std::unique_ptr<Select> lhs, std::unique_ptr<Select> rhs, bool distinct)
        : Statement(lhs->start())
        , m_lhs(std::move(lhs))
        , m_rhs(std::move(rhs))
        , m_distinct(distinct) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::unique_ptr<Select> m_lhs;
    std::unique_ptr<Select> m_rhs;
    bool m_distinct;
};

class DeleteFrom : public Statement {
public:
    DeleteFrom(ssize_t start, std::string from, std::unique_ptr<Expression> where = {})
        : Statement(start)
        , m_from(std::move(from))
        , m_where(std::move(where)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_from;
    std::unique_ptr<Expression> m_where;
};

class Update : public Statement {
public:
    struct UpdatePair {
        std::string column;
        std::unique_ptr<Expression> expr;
    };

    Update(ssize_t start, std::string table, std::vector<UpdatePair> to_update)
        : Statement(start)
        , m_table(table)
        , m_to_update(std::move(to_update)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_table;
    std::vector<UpdatePair> m_to_update;
};

class Import : public Statement {
public:
    enum class Mode {
        Csv
    };

    Import(ssize_t start, Mode mode, std::string filename, std::string table)
        : Statement(start)
        , m_mode(mode)
        , m_filename(std::move(filename))
        , m_table(std::move(table)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    Mode m_mode;
    std::string m_filename;
    std::string m_table;
};

class CreateTable : public Statement {
public:
    CreateTable(ssize_t start, std::string name, std::vector<Column> columns)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
    std::vector<Column> m_columns;
};

class DropTable : public Statement {
public:
    DropTable(ssize_t start, std::string name)
        : Statement(start)
        , m_name(std::move(name)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
};

class TruncateTable : public Statement {
public:
    TruncateTable(ssize_t start, std::string name)
        : Statement(start)
        , m_name(std::move(name)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
};

class AlterTable : public Statement {
public:
    AlterTable(ssize_t start, std::string name, std::vector<Column> to_add, std::vector<Column> to_alter, std::vector<Column> to_drop)
        : Statement(start)
        , m_name(std::move(name))
        , m_to_add(std::move(to_add))
        , m_to_alter(std::move(to_alter))
        , m_to_drop(std::move(to_drop)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
    std::vector<Column> m_to_add;
    std::vector<Column> m_to_alter;
    std::vector<Column> m_to_drop;
};

class InsertInto : public Statement {
public:
    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, std::vector<std::unique_ptr<Core::AST::Expression>> values)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_values(std::move(values)) { }

    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, std::unique_ptr<Core::AST::Select> select)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_select(std::move(select)) { }

    virtual DbErrorOr<Value> execute(Database&) const override;

private:
    std::string m_name;
    std::vector<std::string> m_columns;
    std::vector<std::unique_ptr<Core::AST::Expression>> m_values;
    std::optional<std::unique_ptr<Core::AST::Select>> m_select;
};

}
