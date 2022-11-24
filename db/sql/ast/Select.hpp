#pragma once

#include <db/sql/Select.hpp>
#include <db/sql/ast/Statement.hpp>

namespace Db::Sql::AST {

class SelectExpression : public Expression {
public:
    SelectExpression(ssize_t start, Select select)
        : Expression(start)
        , m_select(std::move(select)) { }

    virtual Core::DbErrorOr<Core::Value> evaluate(EvaluationContext&) const override;
    virtual std::string to_string() const override { return "(SELECT TODO)"; }

private:
    Select m_select;
};

class SelectTableExpression : public TableExpression {
public:
    SelectTableExpression(ssize_t start, Select select)
        : TableExpression(start)
        , m_select(std::move(select)) { }

    virtual Core::DbErrorOr<std::unique_ptr<Core::Relation>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return "(SELECT TODO)"; }
    virtual Core::DbErrorOr<std::optional<size_t>> resolve_identifier(Core::Database* db, Identifier const&) const override;
    virtual Core::DbErrorOr<size_t> column_count(Core::Database* db) const override;

private:
    Select m_select;
};

class SelectStatement : public Statement {
public:
    SelectStatement(ssize_t start, Select select)
        : Statement(start)
        , m_select(std::move(select)) { }

    virtual Core::DbErrorOr<Core::ValueOrResultSet> execute(Core::Database&) const override;

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

    virtual Core::DbErrorOr<Core::ValueOrResultSet> execute(Core::Database&) const override;

private:
    Select m_lhs;
    Select m_rhs;
    bool m_distinct;
};

class InsertInto : public Statement {
public:
    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, std::vector<std::unique_ptr<Sql::AST::Expression>> values)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_values(std::move(values)) { }

    InsertInto(ssize_t start, std::string name, std::vector<std::string> columns, Sql::AST::Select select)
        : Statement(start)
        , m_name(std::move(name))
        , m_columns(std::move(columns))
        , m_select(std::move(select)) { }

    virtual Core::DbErrorOr<Core::ValueOrResultSet> execute(Core::Database&) const override;

private:
    std::string m_name;
    std::vector<std::string> m_columns;
    std::vector<std::unique_ptr<Sql::AST::Expression>> m_values;
    std::optional<Sql::AST::Select> m_select;
};

}
