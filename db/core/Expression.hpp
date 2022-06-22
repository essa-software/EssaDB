#pragma once

#include "db/core/Tuple.hpp"
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <sys/types.h>

namespace Db::Core{
    class Table;
    class Database;
}

namespace Db::Core::AST {

class Expression;
struct EvaluationContext;

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
    Database* db;
    std::optional<std::span<Tuple const>> row_group {};
    enum class RowType {
        FromTable,
        FromResultSet
    };
    RowType row_type;
};

class Expression : virtual public ASTNode {
public:
    explicit Expression(ssize_t start)
        : ASTNode(start) { }

    virtual ~Expression() = default;
    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const = 0;
    virtual std::string to_string() const = 0;
    virtual std::vector<std::string> referenced_columns() const { return {}; }
};

class Literal : public Expression {
public:
    explicit Literal(ssize_t start, Value val)
        : ASTNode(start), Expression(start)
        , m_value(std::move(val)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override { return m_value; }
    virtual std::string to_string() const override { return m_value.to_string().release_value_but_fixme_should_propagate_errors(); }

    Value value() const {return m_value;}

private:
    Value m_value;
};

class Identifier : public Expression {
public:
    explicit Identifier(ssize_t start, std::string id, std::optional<std::string> table)
        : ASTNode(start), Expression(start)
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
        : ASTNode(lhs->start()), Expression(lhs->start())
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
        : ASTNode(lhs->start()), Expression(lhs->start())
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

private:
    DbErrorOr<bool> is_true(EvaluationContext&, Tuple const&) const;

    std::unique_ptr<Expression> m_lhs;
    Operation m_operation {};
    std::unique_ptr<Expression> m_rhs;
};

class BetweenExpression : public Expression {
public:
    BetweenExpression(std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> min, std::unique_ptr<Expression> max)
        : ASTNode(lhs->start()), Expression(lhs->start())
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

private:
    std::unique_ptr<Expression> m_lhs;
    std::unique_ptr<Expression> m_min;
    std::unique_ptr<Expression> m_max;
};

class InExpression : public Expression {
public:
    InExpression(std::unique_ptr<Expression> lhs, std::vector<std::unique_ptr<Core::AST::Expression>> args)
        : ASTNode(lhs->start()), Expression(lhs->start())
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
        : ASTNode(cases.front().expr->start()), Expression(cases.front().expr->start())
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
