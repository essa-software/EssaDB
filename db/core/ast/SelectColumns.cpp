#include <db/core/ast/SelectColumns.hpp>

#include <db/core/ast/EvaluationContext.hpp>
#include <db/core/ast/TableExpression.hpp>

namespace Db::Core::AST {

SelectColumns::SelectColumns(std::vector<Column> columns)
    : m_columns(std::move(columns)) {
    size_t index = 0;
    for (auto const& column : m_columns) {
        if (column.alias)
            m_aliases.insert({ *column.alias, ResolvedAlias { .column = *column.column, .index = index } });
        m_aliases.insert({ column.column->to_string(), ResolvedAlias { .column = *column.column, .index = index } });
        index++;
    }
}

SelectColumns::ResolvedAlias const* SelectColumns::resolve_alias(std::string const& alias) const {
    auto it = m_aliases.find(alias);
    if (it == m_aliases.end()) {
        return nullptr;
    }
    return &it->second;
}

DbErrorOr<Value> SelectColumns::resolve_value(EvaluationContext& context, Identifier const& identifier) const {
    auto const& tuple = context.current_frame().row;
    if (!identifier.table()) {
        auto resolved_alias = resolve_alias(identifier.id());
        if (resolved_alias)
            return tuple.tuple.value(resolved_alias->index);
    }

    if (!tuple.source)
        return DbError { "Cannot use table columns on aggregated rows", 0 };

    std::optional<size_t> index;
    for (auto it = context.frames.rbegin(); it != context.frames.rend(); it++) {
        auto const& frame = *it;
        index = TRY(frame.table->resolve_identifier(context.db, identifier));
        if (index) {
            break;
        }
    }
    if (!index) {
        return DbError { "Invalid identifier", identifier.start() };
    }
    return tuple.source->value(*index);
}

}
