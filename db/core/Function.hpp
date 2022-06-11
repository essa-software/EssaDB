#pragma once
#include "AST.hpp"

namespace Db::Core::AST {

class Function : public Expression {
public:
    explicit Function(size_t start, std::string name, std::vector<std::unique_ptr<Expression>> args)
        : Expression(start)
        , m_name(std::move(name))
        , m_args(std::move(args)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&, Row const&) const override;
    virtual std::string to_string() const override { return m_name + "(TODO)"; }

private:
    std::string m_name;
    std::vector<std::unique_ptr<Expression>> m_args;
};

}
