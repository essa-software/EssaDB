#pragma once

#include "Tuple.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

namespace Db::Core {
class Table;
class Database;
}

namespace Db::Core::AST {

class Expression;
struct EvaluationContext;
class Identifier;

class ASTNode {
public:
    explicit ASTNode(ssize_t start)
        : m_start(start) { }

    auto start() const { return m_start; }

private:
    size_t m_start;
};

struct TupleWithSource {
    Tuple tuple;
    std::optional<Tuple> source;

    DbErrorOr<bool> operator==(TupleWithSource const& other) const {
        return TRY(tuple == other.tuple) && source.has_value() == other.source.has_value() && TRY(source.value() == other.source.value());
    }
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

    ResolvedAlias const* resolve_alias(std::string const& alias) const;
    DbErrorOr<Value> resolve_value(EvaluationContext&, TupleWithSource const&, std::string const& alias) const;

private:
    std::vector<Column> m_columns;
    std::map<std::string, ResolvedAlias> m_aliases;
};

struct EvaluationContext {
    SelectColumns const& columns;
    Table const* table = nullptr;
    Database* db = nullptr;
    std::optional<std::span<Tuple const>> row_group {};
    enum class RowType {
        FromTable,
        FromResultSet
    };
    RowType row_type;
};

class TableExpression : public ASTNode {
public:
    explicit TableExpression(ssize_t start)
        : ASTNode(start) { }

    virtual ~TableExpression() = default;
    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(Database* db) const = 0;
    virtual std::string to_string() const = 0;
    virtual std::vector<std::string> referenced_columns() const { return {}; }

    static Tuple prepare_tuple(const Tuple* lhs_row, const Tuple* rhs_row, size_t index, bool order);
};

class TableIdentifier : public TableExpression {
public:
    explicit TableIdentifier(ssize_t start, std::string id)
        : TableExpression(start)
        , m_id(id) { }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(Database* db) const override;
    virtual std::string to_string() const override { return m_id; }

private:
    std::string m_id;
};

class JoinExpression : public TableExpression {
public:
    enum class Type {
        InnerJoin,
        LeftJoin,
        RightJoin,
        OuterJoin,
        Invalid
    };

    JoinExpression(ssize_t start,
        std::unique_ptr<TableExpression> lhs,
        std::unique_ptr<Identifier> on_lhs,
        Type join_type,
        std::unique_ptr<TableExpression> rhs,
        std::unique_ptr<Identifier> on_rhs)
        : TableExpression(start)
        , m_lhs(std::move(lhs))
        , m_rhs(std::move(rhs))
        , m_on_lhs(std::move(on_lhs))
        , m_on_rhs(std::move(on_rhs))
        , m_join_type(join_type) { }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(Database* db) const override;
    virtual std::string to_string() const override { return "JoinExpression(TODO)"; }

private:
    std::unique_ptr<TableExpression> m_lhs, m_rhs;
    std::unique_ptr<Identifier> m_on_lhs, m_on_rhs;
    Type m_join_type;
};

class CrossJoinExpression : public TableExpression {
public:
    CrossJoinExpression(ssize_t start,
        std::unique_ptr<TableExpression> lhs,
        std::unique_ptr<TableExpression> rhs)
        : TableExpression(start)
        , m_lhs(std::move(lhs))
        , m_rhs(std::move(rhs)){ }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(Database* db) const override;
    virtual std::string to_string() const override { return "JoinExpression(TODO)"; }

private:
    std::unique_ptr<TableExpression> m_lhs, m_rhs;
};

class Expression : public ASTNode {
public:
    explicit Expression(ssize_t start)
        : ASTNode(start) { }

    virtual ~Expression() = default;
    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const = 0;
    virtual std::string to_string() const = 0;
    virtual std::vector<std::string> referenced_columns() const { return {}; }
    virtual bool contains_aggregate_function() const { return false; }

    // run_from will be added to error message.
    DbErrorOr<Value> evaluate_and_require_single_value(EvaluationContext&, TupleWithSource const&, std::string run_from = "") const;
};

class Check : public Expression {
public:
    explicit Check(ssize_t start)
        : Expression(start) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override;
    virtual std::string to_string() const override { return "Check(TODO)"; }

    DbErrorOr<void> add_check(std::shared_ptr<AST::Expression> expr);
    DbErrorOr<void> alter_check(std::shared_ptr<AST::Expression> expr);
    DbErrorOr<void> drop_check();

    DbErrorOr<void> add_constraint(std::string const& name, std::shared_ptr<AST::Expression> expr);
    DbErrorOr<void> alter_constraint(std::string const& name, std::shared_ptr<AST::Expression> expr);
    DbErrorOr<void> drop_constraint(std::string const& name);

    std::shared_ptr<Expression> const& main_rule() const { return m_main_check; }
    std::map<std::string, std::shared_ptr<Expression>> const& constraints() const { return m_constraints; }

private:
    std::shared_ptr<Expression> m_main_check = nullptr;
    std::map<std::string, std::shared_ptr<Expression>> m_constraints;
};

class Literal : public Expression {
public:
    explicit Literal(ssize_t start, Value val)
        : Expression(start)
        , m_value(std::move(val)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override { return m_value; }
    virtual std::string to_string() const override { return m_value.to_string().release_value_but_fixme_should_propagate_errors(); }

    Value value() const { return m_value; }

private:
    Value m_value;
};

class Identifier : public Expression {
public:
    explicit Identifier(ssize_t start, std::string id, std::optional<std::string> table)
        : Expression(start)
        , m_id(std::move(id))
        , m_table(std::move(table)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override;
    virtual std::string to_string() const override { return m_id; }
    virtual std::vector<std::string> referenced_columns() const override { return { m_id }; }

private:
    std::string m_id;
    std::optional<std::string> m_table;
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

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& row) const override {
        return Value::create_bool(TRY(is_true(context, row)));
    }
    virtual std::string to_string() const override { return "BinaryOperator(" + m_lhs->to_string() + "," + m_rhs->to_string() + ")"; }

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        auto rhs_columns = m_rhs->referenced_columns();
        lhs_columns.insert(lhs_columns.begin(), rhs_columns.begin(), rhs_columns.end());
        return lhs_columns;
    }
    virtual bool contains_aggregate_function() const override { return m_lhs->contains_aggregate_function() || m_rhs->contains_aggregate_function(); }

private:
    DbErrorOr<bool> is_true(EvaluationContext&, TupleWithSource const&) const;

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

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& row) const override;

    virtual std::string to_string() const override { return "ArithmeticOperator(" + m_lhs->to_string() + "," + m_rhs->to_string() + ")"; }

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        auto rhs_columns = m_rhs->referenced_columns();
        lhs_columns.insert(lhs_columns.end(), rhs_columns.begin(), rhs_columns.end());
        return lhs_columns;
    }

    virtual bool contains_aggregate_function() const override { return m_lhs->contains_aggregate_function() || m_rhs->contains_aggregate_function(); }

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

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& row) const override;
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

    virtual bool contains_aggregate_function() const override { return m_lhs->contains_aggregate_function() || m_min->contains_aggregate_function() || m_max->contains_aggregate_function(); }

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

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& row) const override;
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

    virtual bool contains_aggregate_function() const override {
        if (m_lhs->contains_aggregate_function())
            return true;
        for (auto const& arg : m_args) {
            if (arg->contains_aggregate_function())
                return true;
        }
        return false;
    }

private:
    std::unique_ptr<Expression> m_lhs;
    std::vector<std::unique_ptr<Core::AST::Expression>> m_args;
};

class IsExpression : public Expression {
public:
    enum class What {
        Null,
        NotNull
    };

    explicit IsExpression(std::unique_ptr<Expression> lhs, What what)
        : Expression(lhs->start())
        , m_lhs(std::move(lhs))
        , m_what(what) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& row) const override;

    virtual std::string to_string() const override {
        return m_lhs->to_string() + " IS " + (m_what == What::Null ? "NULL" : "NOT NULL");
    }

    virtual std::vector<std::string> referenced_columns() const override {
        return m_lhs->referenced_columns();
    }

    virtual bool contains_aggregate_function() const override {
        return m_lhs->contains_aggregate_function();
    }

private:
    std::unique_ptr<Expression> m_lhs;
    What m_what {};
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

    virtual DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& row) const override;
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

    virtual bool contains_aggregate_function() const override {
        if (m_else_value->contains_aggregate_function())
            return true;
        for (auto const& case_ : m_cases) {
            if (case_.expr->contains_aggregate_function())
                return true;
        }
        return false;
    }

private:
    std::vector<CasePair> m_cases;

    std::unique_ptr<Core::AST::Expression> m_else_value;
};

class ExpressionOrIndex : public std::variant<size_t, std::unique_ptr<Expression>> {
public:
    using Variant = std::variant<size_t, std::unique_ptr<Expression>>;

    explicit ExpressionOrIndex(size_t index)
        : Variant { index } { }

    explicit ExpressionOrIndex(std::unique_ptr<Expression> expr)
        : Variant(std::move(expr)) { }

    bool is_index() const { return std::holds_alternative<size_t>(*this); }
    size_t index() const { return std::get<size_t>(*this); }

    bool is_expression() const { return std::holds_alternative<std::unique_ptr<Expression>>(*this); }
    Expression& expression() const { return *std::get<std::unique_ptr<Expression>>(*this); }

    DbErrorOr<Value> evaluate(EvaluationContext& context, TupleWithSource const& input) const;
};

}
