#include "db/sql/SQLError.hpp"
#include <db/sql/ast/Select.hpp>

#include <db/core/TupleFromValues.hpp>

namespace Db::Sql::AST {

SQLErrorOr<Core::Value> SelectExpression::evaluate(EvaluationContext& context) const {
    auto result_set = TRY(m_select.execute(context));
    if (!result_set.is_convertible_to_value()) {
        return SQLError { "Select expression must return a single row with a single value", start() };
    }
    return result_set.as_value();
}

SQLErrorOr<std::unique_ptr<Core::Relation>> SelectTableExpression::evaluate(EvaluationContext& context) const {
    if (!context.db) {
        return SQLError { "SELECT run as table expression requires a database", start() };
    }
    auto result = TRY(m_select.execute(context));

    auto table = std::make_unique<Core::MemoryBackedTable>(nullptr, "");

    size_t i = 0;
    for (const auto& name : result.column_names()) {
        Core::Value::Type type = result.rows().empty() ? Core::Value::Type::Null : result.rows().front().value(i).type();
        TRY(table->add_column(Core::Column(name, type, 0, 0, 0)).map_error(DbToSQLError { start() }));
        i++;
    }

    for (const auto& row : result.rows()) {
        TRY(table->insert(context.db, row).map_error(DbToSQLError { start() }));
    }

    return table;
}

SQLErrorOr<Core::ValueOrResultSet> SelectStatement::execute(Core::Database& db) const {
    EvaluationContext context { .db = &db };
    return TRY(m_select.execute(context));
}

SQLErrorOr<Core::ValueOrResultSet> Union::execute(Core::Database& db) const {
    EvaluationContext context { .db = &db };
    auto lhs = TRY(m_lhs.execute(context));
    auto rhs = TRY(m_rhs.execute(context));

    if (lhs.column_names().size() != rhs.column_names().size())
        return SQLError { "Queries with different column count", 0 };

    for (size_t i = 0; i < lhs.column_names().size(); i++) {
        if (lhs.column_names()[i] != rhs.column_names()[i])
            return SQLError { "Queries with different column set", 0 };
    }

    std::vector<Core::Tuple> rows;

    for (const auto& row : lhs.rows()) {
        rows.push_back(row);
    }

    for (const auto& row : rhs.rows()) {
        if (m_distinct) {
            bool distinct = true;
            for (const auto& to_compare : lhs.rows()) {
                if (TRY((row == to_compare).map_error(DbToSQLError { start() }))) {
                    distinct = false;
                    break;
                }
            }

            if (distinct)
                rows.push_back(row);
        }
        else {
            rows.push_back(row);
        }
    }

    return Core::ResultSet { lhs.column_names(), std::move(rows) };
}

SQLErrorOr<std::optional<size_t>> SelectTableExpression::resolve_identifier(Core::Database* db, Identifier const& id) const {
    assert(m_select.from());
    return m_select.from()->resolve_identifier(db, id);
}

SQLErrorOr<size_t> SelectTableExpression::column_count(Core::Database* db) const {
    assert(m_select.from());
    return m_select.from()->column_count(db);
}

SQLErrorOr<Core::ValueOrResultSet> InsertInto::execute(Core::Database& db) const {
    auto table = TRY(db.table(m_name).map_error(DbToSQLError { start() }));

    EvaluationContext context { .db = &db };
    if (m_select) {
        auto result = TRY(m_select.value().execute(context));
        for (const auto& row : result.rows()) {
            if (m_columns.empty()) {
                std::vector<Core::Value> values;
                for (size_t s = 0; s < table->size(); s++) {
                    values.push_back(row.value(s));
                }
                TRY(table->insert(&db, Core::Tuple { values }).map_error(DbToSQLError { start() }));
            }
            else {
                std::vector<std::pair<std::string, Core::Value>> values;
                for (size_t i = 0; i < m_columns.size(); i++) {
                    values.push_back({ m_columns[i], row.value(i) });
                }
                TRY(table->insert(&db, TRY(create_tuple_from_values(*table, values).map_error(DbToSQLError { start() })))
                        .map_error(DbToSQLError { start() }));
            }
        }
    }
    else {
        if (m_columns.empty()) {
            std::vector<Core::Value> values;
            for (size_t s = 0; s < m_values.size(); s++) {
                values.push_back(TRY(m_values[s]->evaluate(context)));
            }
            TRY(table->insert(&db, Core::Tuple { values }).map_error(DbToSQLError { start() }));
        }
        else {
            std::vector<std::pair<std::string, Core::Value>> values;
            for (size_t i = 0; i < m_columns.size(); i++) {
                values.push_back({ m_columns[i], TRY(m_values[i]->evaluate(context)) });
            }

            TRY(table->insert(&db, TRY(create_tuple_from_values(*table, values).map_error(DbToSQLError { start() })))
                    .map_error(DbToSQLError { start() }));
        }
    }
    return { Core::Value::null() };
}

}
