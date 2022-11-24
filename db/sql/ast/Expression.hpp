#pragma once

#include <db/core/Tuple.hpp>
#include <db/sql/ast/ASTNode.hpp>
#include <list>
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

namespace Db::Sql::AST {

class Expression;
struct EvaluationContext;
class Identifier;

class Expression : public ASTNode {
public:
    explicit Expression(ssize_t start)
        : ASTNode(start) { }

    virtual ~Expression() = default;
    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const = 0;
    virtual std::string to_string() const = 0;
    virtual std::vector<std::string> referenced_columns() const { return {}; }
    virtual bool contains_aggregate_function() const { return false; }
};

class Check : public Expression {
public:
    explicit Check(ssize_t start)
        : Expression(start) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override { return "Check(TODO)"; }

    Core::DbErrorOr<void> add_check(std::shared_ptr<AST::Expression> expr);
    Core::DbErrorOr<void> alter_check(std::shared_ptr<AST::Expression> expr);
    Core::DbErrorOr<void> drop_check();

    Core::DbErrorOr<void> add_constraint(std::string const& name, std::shared_ptr<AST::Expression> expr);
    Core::DbErrorOr<void> alter_constraint(std::string const& name, std::shared_ptr<AST::Expression> expr);
    Core::DbErrorOr<void> drop_constraint(std::string const& name);

    std::shared_ptr<Expression> const& main_rule() const { return m_main_check; }
    std::map<std::string, std::shared_ptr<Expression>> const& constraints() const { return m_constraints; }

private:
    std::shared_ptr<Expression> m_main_check = nullptr;
    std::map<std::string, std::shared_ptr<Expression>> m_constraints;
};

class Literal : public Expression {
public:
    explicit Literal(ssize_t start, Core::Value val)
        : Expression(start)
        , m_value(std::move(val)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override { return m_value; }
    virtual std::string to_string() const override { return m_value.to_string().release_value_but_fixme_should_propagate_errors(); }

    Core::Value value() const { return m_value; }

private:
    Core::Value m_value;
};

class Identifier : public Expression {
public:
    explicit Identifier(ssize_t start, std::string id, std::optional<std::string> table)
        : Expression(start)
        , m_id(std::move(id))
        , m_table(std::move(table)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override { return m_id; }
    virtual std::vector<std::string> referenced_columns() const override { return { m_id }; }

    std::string id() const { return m_id; }
    auto table() const { return m_table; }

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
        Match,
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

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext& context) const override {
        return Core::Value::create_bool(TRY(is_true(context)));
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
    Core::DbErrorOr<bool> is_true(EvaluationContext&) const;

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

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext& context) const override;

    virtual std::string to_string() const override;

    virtual std::vector<std::string> referenced_columns() const override {
        auto lhs_columns = m_lhs->referenced_columns();
        auto rhs_columns = m_rhs->referenced_columns();
        lhs_columns.insert(lhs_columns.end(), rhs_columns.begin(), rhs_columns.end());
        return lhs_columns;
    }

    virtual bool contains_aggregate_function() const override { return m_lhs->contains_aggregate_function() || m_rhs->contains_aggregate_function(); }

private:
    std::unique_ptr<Expression> m_lhs;
    Operation m_operation {};
    std::unique_ptr<Expression> m_rhs;
};

class UnaryOperator : public Expression {
public:
    enum class Operation {
        Minus
    };

    UnaryOperator(Operation op, std::unique_ptr<Expression> operand)
        : Expression(operand->start() - 1)
        , m_operation(op)
        , m_operand(std::move(operand)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext& context) const override;

    virtual std::string to_string() const override { return "UnaryOperator(" + m_operand->to_string() + ")"; }

    virtual std::vector<std::string> referenced_columns() const override {
        return m_operand->referenced_columns();
    }

    virtual bool contains_aggregate_function() const override {
        return m_operand->contains_aggregate_function();
    }

private:
    Operation m_operation {};
    std::unique_ptr<Expression> m_operand;
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

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
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
    InExpression(std::unique_ptr<Expression> lhs, std::vector<std::unique_ptr<Expression>> args)
        : Expression(lhs->start())
        , m_lhs(std::move(lhs))
        , m_args(std::move(args)) {
        assert(m_lhs);
    }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
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
    std::vector<std::unique_ptr<Expression>> m_args;
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

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;

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

    CaseExpression(std::vector<CasePair> cases, std::unique_ptr<Expression> else_value = {})
        : Expression(cases.front().expr->start())
        , m_cases(std::move(cases))
        , m_else_value(std::move(else_value)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
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

    std::unique_ptr<Expression> m_else_value;
};

class NonOwningExpressionProxy : public Expression {
public:
    NonOwningExpressionProxy(ssize_t start, Expression const& expr)
        : Expression(start)
        , m_expression(expr) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override;
    virtual std::vector<std::string> referenced_columns() const override;
    virtual bool contains_aggregate_function() const override;

private:
    Expression const& m_expression;
};

class IndexExpression : public Expression {
public:
    IndexExpression(ssize_t start, size_t idx, std::string name)
        : Expression(start)
        , m_index(idx)
        , m_name(std::move(name)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override;
    virtual std::vector<std::string> referenced_columns() const override;

private:
    size_t m_index = 0;
    std::string m_name;
};

}
