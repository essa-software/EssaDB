#pragma once

#include "AST.hpp"

namespace Db::Core::AST {

class TableExpression : public ASTNode {
public:
    explicit TableExpression(ssize_t start)
        : ASTNode(start) { }

    virtual ~TableExpression() = default;
    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(EvaluationContext& context) const = 0;
    virtual std::string to_string() const = 0;
    virtual DbErrorOr<std::optional<size_t>> resolve_identifier(Database* db, Identifier const&) const = 0;
    virtual DbErrorOr<size_t> column_count(Database*) const = 0;

    static Tuple create_joined_tuple(const Tuple& lhs_row, const Tuple& rhs_row);
};

class SimpleTableExpression : public TableExpression {
public:
    explicit SimpleTableExpression(ssize_t start, Table const& table)
        : TableExpression(start)
        , m_table(table) { }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override;
    virtual DbErrorOr<std::optional<size_t>> resolve_identifier(Database* db, Identifier const&) const override;
    virtual DbErrorOr<size_t> column_count(Database* db) const override;

private:
    Table const& m_table;
};

class TableIdentifier : public TableExpression {
public:
    explicit TableIdentifier(ssize_t start, std::string id, std::optional<std::string> alias)
        : TableExpression(start)
        , m_id(std::move(id))
        , m_alias(std::move(alias)) { }

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return m_id; }
    virtual DbErrorOr<std::optional<size_t>> resolve_identifier(Database* db, Identifier const&) const override;
    virtual DbErrorOr<size_t> column_count(Database* db) const override;

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

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return "JoinExpression(TODO)"; }
    virtual DbErrorOr<std::optional<size_t>> resolve_identifier(Database* db, Identifier const&) const override;
    virtual DbErrorOr<size_t> column_count(Database* db) const override;

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

    virtual DbErrorOr<std::unique_ptr<Table>> evaluate(EvaluationContext& context) const override;
    virtual std::string to_string() const override { return "JoinExpression(TODO)"; }
    virtual DbErrorOr<std::optional<size_t>> resolve_identifier(Database* db, Identifier const&) const override;
    virtual DbErrorOr<size_t> column_count(Database* db) const override;

private:
    std::unique_ptr<TableExpression> m_lhs, m_rhs;
};

}
