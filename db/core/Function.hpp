#pragma once
#include "AST.hpp"

namespace Db::Core::AST {

class Function : public Expression {
public:
    explicit Function(size_t start, std::string name, std::vector<std::unique_ptr<Expression>> args)
        : Expression(start)
        , m_name(std::move(name))
        , m_args(std::move(args)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Tuple const&) const override;
    virtual std::string to_string() const override { return m_name + "(TODO)"; }

private:
    std::string m_name;
    std::vector<std::unique_ptr<Expression>> m_args;
};

class AggregateFunction : public Expression {
public:
    enum class Function {
        Count,
        Sum,
        Invalid
    };

    explicit AggregateFunction(size_t start, Function function, std::string column)
        : Expression(start)
        , m_function(function)
        , m_column(std::move(column)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Tuple const&) const override;
    virtual std::string to_string() const override { return "AggregateFunction?(TODO)"; }

    DbErrorOr<Value> aggregate(EvaluationContext&, std::vector<Tuple> const&) const;

private:
    Function m_function {};
    std::string m_column;
};

}
