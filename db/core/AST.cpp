#include "AST.hpp"

#include "AbstractTable.hpp"
#include "Database.hpp"
#include "DbError.hpp"
#include "Expression.hpp"
#include "Function.hpp"
#include "RowWithColumnNames.hpp"
#include "Select.hpp"
#include "Tuple.hpp"
#include "Value.hpp"

#include <EssaUtil/Is.hpp>
#include <cctype>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Db::Core::AST {

DbErrorOr<ValueOrResultSet> DeleteFrom::execute(Database& db) const {
    auto table = TRY(db.table(m_from));

    EvaluationContext context { .columns = {}, .table = table, .db = &db, .row_type = EvaluationContext::RowType::FromTable };

    // TODO: Implement
    auto should_include_row = [&](Tuple const& row) -> DbErrorOr<bool> {
        if (!m_where)
            return true;
        return TRY(m_where->evaluate(context, { .tuple = row, .source = {} })).to_bool();
    };

    // TODO: Maintain integrity on error
    std::optional<DbError> error;
    TRY(table->rows_writable().try_for_each_row([&](TupleWriter const& writer) -> DbErrorOr<void> {
        auto result = should_include_row(writer.read());
        if (result.is_error())
            return {};
        if (result.release_value())
            TRY(writer.remove());
        return {};
    }));
    if (error)
        return *error;
    return Value::null();
}

DbErrorOr<ValueOrResultSet> Update::execute(Database& db) const {
    auto table = TRY(db.table(m_table));

    EvaluationContext context { .columns = {}, .table = table, .db = &db, .row_type = EvaluationContext::RowType::FromTable };

    for (const auto& update_pair : m_to_update) {
        auto column = table->get_column(update_pair.column);
        TRY(table->rows_writable().try_for_each_row([&](TupleWriter const& row) -> DbErrorOr<void> {
            auto tuple = row.read();
            tuple.set_value(column->index, TRY(update_pair.expr->evaluate(context, { .tuple = tuple, .source = {} })));
            TRY(row.write(tuple));
            return {};
        }));
    }

    return Value::null();
}

DbErrorOr<ValueOrResultSet> CreateTable::execute(Database& db) const {
    auto& table = db.create_table(m_name, m_check);
    for (auto const& column : m_columns) {
        TRY(table.add_column(column));
    }
    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> DropTable::execute(Database& db) const {
    TRY(db.drop_table(m_name));

    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> TruncateTable::execute(Database& db) const {
    auto table = TRY(db.table(m_name));
    TRY(table->truncate());

    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> AlterTable::execute(Database& db) const {
    auto table = TRY(db.table(m_name));

    for (const auto& to_add : m_to_add) {
        TRY(table->add_column(to_add));
    }

    for (const auto& to_alter : m_to_alter) {
        TRY(table->alter_column(to_alter));
    }

    for (const auto& to_drop : m_to_drop) {
        TRY(table->drop_column(to_drop.name()));
    }

    auto memory_backed_table = dynamic_cast<MemoryBackedTable*>(table);

    if (!memory_backed_table)
        return DbError { "Table with invalid type!", 0 };

    auto check_exists = [&]() -> DbErrorOr<bool> {
        if (memory_backed_table->check())
            return true;
        return DbError { "No check to alter!", 0 };
    };

    if (m_check_to_add && TRY(check_exists()))
        TRY(memory_backed_table->check()->add_check(m_check_to_add));

    if (m_check_to_alter && TRY(check_exists()))
        TRY(memory_backed_table->check()->alter_check(m_check_to_alter));

    if (m_check_to_drop && TRY(check_exists()))
        TRY(memory_backed_table->check()->drop_check());

    for (const auto& to_add : m_constraint_to_add) {
        if (TRY(check_exists()))
            TRY(memory_backed_table->check()->add_constraint(to_add.first, to_add.second));
    }

    for (const auto& to_alter : m_constraint_to_alter) {
        if (TRY(check_exists()))
            TRY(memory_backed_table->check()->alter_constraint(to_alter.first, to_alter.second));
    }

    for (const auto& to_drop : m_constraint_to_drop) {
        if (TRY(check_exists()))
            TRY(memory_backed_table->check()->drop_constraint(to_drop));
    }

    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> InsertInto::execute(Database& db) const {
    auto table = TRY(db.table(m_name));

    RowWithColumnNames::MapType map;
    EvaluationContext context { .columns = {}, .table = table, .db = &db, .row_type = EvaluationContext::RowType::FromTable };
    if (m_select) {
        auto result = TRY(m_select.value().execute(db));

        if (m_columns.size() != result.column_names().size())
            return DbError { "Values doesn't have corresponding columns", start() };

        for (const auto& row : result.rows()) {
            for (size_t i = 0; i < m_columns.size(); i++) {
                map.insert({ m_columns[i], row.value(i) });
            }

            TRY(table->insert(std::move(map)));
        }
    }
    else {
        if (m_columns.size() != m_values.size())
            return DbError { "Values doesn't have corresponding columns", start() };

        for (size_t i = 0; i < m_columns.size(); i++) {
            map.insert({ m_columns[i], TRY(m_values[i]->evaluate(context, {})) });
        }

        TRY(table->insert(std::move(map)));
    }
    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> Import::execute(Database& db) const {
    TRY(db.import_to_table(m_filename, m_table, m_mode));
    return Value::null();
}

}
