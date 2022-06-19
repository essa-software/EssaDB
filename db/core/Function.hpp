#pragma once
#include "AST.hpp"

namespace Db::Core::AST {

class Function : public Expression {
public:
    explicit Function(size_t start, std::string name, std::vector<std::unique_ptr<Expression>> args)
        : Expression(start)
        , m_name(std::move(name))
        , m_args(std::move(args)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override;
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

    explicit AggregateFunction(size_t start, Function function, std::string column)
        : Expression(start)
        , m_function(function)
        , m_column(std::move(column)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, TupleWithSource const&) const override;
    virtual std::string to_string() const override { return "AggregateFunction?(TODO)"; }

    DbErrorOr<Value> aggregate(EvaluationContext&, std::vector<Tuple> const&) const;

    virtual std::vector<std::string> referenced_columns() const override { return { m_column }; }

private:
    Function m_function {};
    std::string m_column;
};

}
