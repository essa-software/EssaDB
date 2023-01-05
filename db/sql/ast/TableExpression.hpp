#pragma once

#include <db/core/Table.hpp>
#include <db/sql/ast/ASTNode.hpp>
#include <db/sql/ast/EvaluationContext.hpp>
#include <db/sql/ast/Expression.hpp>
#include <memory>

namespace Db::Sql::AST {

class TableExpression : public ASTNode {
public:
    explicit TableExpression(ssize_t start)
        : ASTNode(start) { }

    virtual ~TableExpression() = default;
    virtual SQLErrorOr<std::unique_ptr<Core::Relation>> evaluate(EvaluationContext& context) const = 0;
    virtual std::string to_string() const = 0;
    virtual SQLErrorOr<std::optional<size_t>> resolve_identifier(Core::Database* db, Identifier const&) const = 0;
    virtual SQLErrorOr<size_t> column_count(Core::Database*) const = 0;

    static Core::Tuple create_joined_tuple(const Core::Tuple& lhs_row, const Core::Tuple& rhs_row);
};

class SimpleTableExpression : public TableExpression {
public:
    explicit SimpleTableExpression(ssize_t start, Core::Table const& table)
        : TableExpression(start)
        , m_table(table) { }

    virtual SQLErrorOr<std::unique_ptr<Core::Relation>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override;
    virtual SQLErrorOr<std::optional<size_t>> resolve_identifier(Core::Database* db, Identifier const&) const override;
    virtual SQLErrorOr<size_t> column_count(Core::Database* db) const override;

private:
    Core::Table const& m_table;
};

class TableIdentifier : public TableExpression {
public:
    explicit TableIdentifier(ssize_t start, std::string id, std::optional<std::string> alias)
        : TableExpression(start)
        , m_id(std::move(id))
        , m_alias(std::move(alias)) { }

    virtual SQLErrorOr<std::unique_ptr<Core::Relation>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return m_id; }
    virtual SQLErrorOr<std::optional<size_t>> resolve_identifier(Core::Database* db, Identifier const&) const override;
    virtual SQLErrorOr<size_t> column_count(Core::Database* db) const override;

private:
    std::string m_id;
    std::optional<std::string> m_alias;
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

    virtual SQLErrorOr<std::unique_ptr<Core::Relation>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override;
    virtual SQLErrorOr<std::optional<size_t>> resolve_identifier(Core::Database* db, Identifier const&) const override;
    virtual SQLErrorOr<size_t> column_count(Core::Database* db) const override;

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
        , m_rhs(std::move(rhs)) { }

    virtual SQLErrorOr<std::unique_ptr<Core::Relation>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return "JoinExpression(TODO)"; }
    virtual SQLErrorOr<std::optional<size_t>> resolve_identifier(Core::Database* db, Identifier const&) const override;
    virtual SQLErrorOr<size_t> column_count(Core::Database* db) const override;

private:
    std::unique_ptr<TableExpression> m_lhs, m_rhs;
};

}
