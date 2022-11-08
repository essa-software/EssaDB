#include <db/core/ast/Select.hpp>

#include <db/core/TupleFromValues.hpp>

namespace Db::Core::AST {

DbErrorOr<Value> SelectExpression::evaluate(EvaluationContext& context) const {
    auto result_set = TRY(m_select.execute(context));
    if (!result_set.is_convertible_to_value()) {
        // TODO: Store location info
        return DbError { "Select expression must return a single row with a single value", 0 };
    }
    return result_set.as_value();
}

DbErrorOr<std::unique_ptr<Relation>> SelectTableExpression::evaluate(EvaluationContext& context) const {
    auto result = TRY(m_select.execute(context));

    auto table = std::make_unique<MemoryBackedTable>(nullptr, "");

    size_t i = 0;
    for (const auto& name : result.column_names()) {
        Value::Type type = result.rows().empty() ? Value::Type::Null : result.rows().front().value(i).type();
        TRY(table->add_column(Column(name, type, 0, 0, 0)));
        i++;
    }

    for (const auto& row : result.rows()) {
        TRY(table->insert(row));
    }

    return table;
}

DbErrorOr<ValueOrResultSet> SelectStatement::execute(Database& db) const {
    EvaluationContext context { .db = &db };
    return TRY(m_select.execute(context));
}

DbErrorOr<ValueOrResultSet> Union::execute(Database& db) const {
    EvaluationContext context { .db = &db };
    auto lhs = TRY(m_lhs.execute(context));
    auto rhs = TRY(m_rhs.execute(context));

    if (lhs.column_names().size() != rhs.column_names().size())
        return DbError { "Queries with different column count", 0 };

    for (size_t i = 0; i < lhs.column_names().size(); i++) {
        if (lhs.column_names()[i] != rhs.column_names()[i])
            return DbError { "Queries with different column set", 0 };
    }

    std::vector<Tuple> rows;

    for (const auto& row : lhs.rows()) {
        rows.push_back(row);
    }

    for (const auto& row : rhs.rows()) {
        if (m_distinct) {
            bool distinct = true;
            for (const auto& to_compare : lhs.rows()) {
                if (TRY(row == to_compare)) {
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

    return ResultSet { lhs.column_names(), std::move(rows) };
}

DbErrorOr<std::optional<size_t>> SelectTableExpression::resolve_identifier(Database* db, Identifier const& id) const {
    assert(m_select.from());
    return m_select.from()->resolve_identifier(db, id);
}

DbErrorOr<size_t> SelectTableExpression::column_count(Database* db) const {
    assert(m_select.from());
    return m_select.from()->column_count(db);
}

DbErrorOr<ValueOrResultSet> InsertInto::execute(Database& db) const {
    auto table = TRY(db.table(m_name));

    EvaluationContext context { .db = &db };
    if (m_select) {
        auto result = TRY(m_select.value().execute(context));

        if (m_columns.size() != result.column_names().size())
            return DbError { "Values doesn't have corresponding columns", start() };

        for (const auto& row : result.rows()) {
            std::vector<std::pair<std::string, Value>> values;
            for (size_t i = 0; i < m_columns.size(); i++) {
                values.push_back({ m_columns[i], row.value(i) });
            }

            TRY(table->insert(TRY(create_tuple_from_values(db, *table, values))));
        }
    }
    else {
        if (m_columns.size() != m_values.size())
            return DbError { "Values doesn't have corresponding columns", start() };

        std::vector<std::pair<std::string, Value>> values;
        for (size_t i = 0; i < m_columns.size(); i++) {
            values.push_back({ m_columns[i], TRY(m_values[i]->evaluate(context)) });
        }

        TRY(table->insert(TRY(create_tuple_from_values(db, *table, values))));
    }
    return { Value::null() };
}

}
