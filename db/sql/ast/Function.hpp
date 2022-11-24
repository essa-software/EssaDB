#pragma once

#include <db/sql/ast/Expression.hpp>

namespace Db::Sql::AST {

class Function : public Expression {
public:
    explicit Function(size_t start, std::string name, std::vector<std::unique_ptr<Expression>> args)
        : Expression(start)
        , m_name(std::move(name))
        , m_args(std::move(args)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override { return m_name + "(TODO)"; }

    virtual std::vector<std::string> referenced_columns() const override {
        std::vector<std::string> columns;
        for (auto const& arg : m_args) {
            auto arg_columns = arg->referenced_columns();
            columns.insert(columns.end(), arg_columns.begin(), arg_columns.end());
        }
        return columns;
    }

private:
    std::string m_name;
    std::vector<std::unique_ptr<Expression>> m_args;
};

class AggregateFunction : public Expression {
public:
    enum class Function {
        Count,
        Sum,
        Min,
        Max,
        Avg,
        Invalid
    };

    explicit AggregateFunction(size_t start, Function function, std::unique_ptr<Expression> expression, std::optional<std::string> over)
        : Expression(start)
        , m_function(function)
        , m_expression(std::move(expression))
        , m_over(std::move(over)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override { return "AggregateFunction?(TODO)"; }

    Core::DbErrorOr<Core::Value> aggregate(EvaluationContext&, std::span<Core::Tuple const> rows) const;

    virtual std::vector<std::string> referenced_columns() const override { return m_expression->referenced_columns(); }
    virtual bool contains_aggregate_function() const override { return true; }

private:
    Function m_function {};
    std::unique_ptr<Expression> m_expression;
    std::optional<std::string> m_over;
};

}
