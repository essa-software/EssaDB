#pragma once

#include <db/core/Select.hpp>

namespace Db::Core::AST {

class SelectExpression : public Expression {
public:
    SelectExpression(ssize_t start, Select select)
        : Expression(start)
        , m_select(std::move(select)) { }

    virtual DbErrorOr<Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override { return "(SELECT TODO)"; }

private:
    Select m_select;
};

class SelectTableExpression : public TableExpression {
public:
    SelectTableExpression(ssize_t start, Select select)
        : TableExpression(start)
        , m_select(std::move(select)) { }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return "(SELECT TODO)"; }
    virtual DbErrorOr<std::optional<size_t>> resolve_identifier(Database* db, Identifier const&) const override;
    virtual DbErrorOr<size_t> column_count(Database* db) const override;

private:
    Select m_select;
};

class SelectStatement : public Statement {
public:
    SelectStatement(ssize_t start, Select select)
        : Statement(start)
        , m_select(std::move(select)) { }

    virtual DbErrorOr<ValueOrResultSet> execute(Database&) const override;

private:
    Select m_select;
};

class Union : public Statement {
public:
    Union(ssize_t start, Select lhs, Select rhs, bool distinct)
        : Statement(start)
        , m_lhs(std::move(lhs))
        , m_rhs(std::move(rhs))
        , m_distinct(distinct) { }

    virtual DbErrorOr<ValueOrResultSet> execute(Database&) const override;

private:
    Select m_lhs;
    Select m_rhs;
    bool m_distinct;
};

class InsertInto : public Statement {
public:
    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, std::vector<std::unique_ptr<Core::AST::Expression>> values)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_values(std::move(values)) { }

    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, Core::AST::Select select)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_select(std::move(select)) { }

    virtual DbErrorOr<ValueOrResultSet> execute(Database&) const override;

private:
    std::string m_name;
    std::vector<std::string> m_columns;
    std::vector<std::unique_ptr<Core::AST::Expression>> m_values;
    std::optional<Core::AST::Select> m_select;
};

}
