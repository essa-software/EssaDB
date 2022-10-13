#pragma once

#include <db/core/Value.hpp>
#include <db/core/ast/Expression.hpp>
#include <memory>
#include <optional>
#include <string>

namespace Db::Core::AST {

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
    DbErrorOr<Value> resolve_value(EvaluationContext&, Identifier const&) const;

private:
    std::vector<Column> m_columns;
    std::map<std::string, ResolvedAlias> m_aliases;
};

}
