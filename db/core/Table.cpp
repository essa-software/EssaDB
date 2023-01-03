#include "Table.hpp"

#include <cstring>
#include <db/core/Column.hpp>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>
#include <db/core/ResultSet.hpp>
#include <db/core/TupleFromValues.hpp>
#include <db/storage/CSVFile.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// FIXME: Get rid of Core -> SQL dependency
#include <db/sql/ast/EvaluationContext.hpp>
#include <db/sql/ast/Expression.hpp>
#include <db/sql/ast/TableExpression.hpp>

namespace Db::Core {

DbErrorOr<void> Table::check_value_validity(Database&, Tuple const& row, size_t column_index) const {
    auto const& column = columns()[column_index];
    if (!row.value(column_index).is_null() && column.type() != row.value(column_index).type()) {
        return DbError {
            fmt::format("Type mismatch, required {} but given {} for column '{}'",
                Value::type_to_string(column.type()),
                Value::type_to_string(row.value(column_index).type()),
                column.name()),
        };
    }

    if (column.not_null() && row.value(column_index).is_null()) {
        return DbError { fmt::format("NULL given for NOT NULL column '{}'", column.name()) };
    }

    if (column.unique()) {
        TRY(rows().try_for_each_row([&](Tuple const& other_row) -> DbErrorOr<void> {
            if (TRY(other_row.value(column_index) == row.value(column_index)))
                return DbError { fmt::format("Column '{}' must contain unique values", column.name()) };
            return {};
        }));
    }

    return {};
}

DbErrorOr<void> Table::perform_table_integrity_checks(Database& db, Tuple const& row) const {
    auto const& columns = this->columns();

    // Column count
    if (row.value_count() != columns.size()) {
        return DbError { fmt::format("Column count does not match ({} given vs {} required)", row.value_count(), columns.size()) };
    }

    // Primary key (NULL + duplicate check)
    // For now, this is checked even if no constraint is explicitly defined.
    if (primary_key()) {
        auto const& pk = primary_key();
        auto column = get_column(pk->local_column);
        if (!column) {
            return DbError { fmt::format("Internal error: Nonexistent column '{}' used as primary key", pk->local_column) };
        }
        auto const& value = row.value(column->index);
        if (value.is_null()) {
            return DbError { "Primary key may not be null" };
        }
        TRY(rows().try_for_each_row([&](Tuple const& other_row) -> DbErrorOr<void> {
            if (TRY(other_row.value(column->index) == value))
                return DbError { "Primary key must be unique" };
            return {};
        }));
    }

    // Column types, NON NULL, UNIQUE
    for (size_t s = 0; s < columns.size(); s++) {
        TRY(check_value_validity(db, row, s));
    }

    return {};
}

DbErrorOr<void> Table::perform_database_integrity_checks(Database& db, Tuple const& row) const {
    // Foreign keys (row existence in the foreign table)
    // For now, this is checked even if no constraint is explicitly defined.
    for (auto const& fk : foreign_keys()) {
        auto local_column = get_column(fk.local_column);
        if (!local_column) {
            return DbError { fmt::format("Internal error: Nonexistent column '{}' used as foreign key", fk.local_column) };
        }
        auto referenced_table = TRY(db.table(fk.referenced_table));
        auto referenced_column = referenced_table->get_column(fk.referenced_column);
        if (!referenced_column) {
            return DbError { fmt::format("Internal error: Nonexistent column '{}' used as referenced table in foreign key", fk.referenced_column) };
        }

        auto const& local_value = row.value(local_column->index);
        if (local_value.is_null()) {
            continue;
        }

        if (!referenced_table->find_first_matching_tuple(referenced_column->index, local_value)) {
            return DbError {
                fmt::format("Foreign key '{}' requires matching value in referenced column '{}.{}'",
                    fk.local_column, fk.referenced_table, fk.referenced_column),
            };
        }
    }

    return {};
}

DbErrorOr<void> Table::insert(Database& db, Tuple const& row) {
    auto const& columns = this->columns();

    std::vector<std::string> columns_to_auto_increment;

    // Filling (AUTO_INCREMENT and default values)
    Tuple filled_row = row;
    for (size_t s = 0; s < row.value_count(); s++) {
        auto value = filled_row.value(s);
        if (value.is_null()) {
            if (columns[s].auto_increment()) {
                if (columns[s].type() == Value::Type::Int) {
                    filled_row.set_value(s, Value::create_int(next_auto_increment_value(columns[s].name())));
                    columns_to_auto_increment.push_back(columns[s].name());
                }
                else
                    return DbError { "Internal error: AUTO_INCREMENT used on non-int field" };
            }
            else {
                filled_row.set_value(s, columns[s].default_value());
            }
        }
    }
    // fmt::print("{}\n", filled_row.value(0).to_debug_string());

    TRY(perform_table_integrity_checks(db, filled_row));
    TRY(perform_database_integrity_checks(db, filled_row));

    for (auto const& column : columns_to_auto_increment) {
        increment(column);
    }

    return insert_unchecked(filled_row);
}

DbErrorOr<std::unique_ptr<MemoryBackedTable>> MemoryBackedTable::create_from_select_result(ResultSet const& select) {
    std::unique_ptr<MemoryBackedTable> table = std::make_unique<MemoryBackedTable>(nullptr, "");

    auto const& columns = select.column_names();
    auto const& rows = select.rows();

    size_t i = 0;
    for (const auto& col : columns) {
        TRY(table->add_column(Column(col, rows[0].value(i).type(), false, false, false)));
        i++;
    }
    table->m_rows = rows;
    return table;
}

DbErrorOr<void> MemoryBackedTable::add_column(Column column) {
    if (!column.original_table())
        column.set_table(this);
    m_columns.push_back(std::move(column));

    for (auto& row : m_rows) {
        row.extend();
    }

    return {};
}

DbErrorOr<void> MemoryBackedTable::alter_column(Column column) {
    if (!get_column(column.name())) {
        return DbError { "Couldn't find column '" + column.name() + "'" };
    }

    for (size_t i = 0; i < m_columns.size(); i++) {
        if (m_columns[i].name() == column.name()) {
            m_columns[i] = std::move(column);
        }
    }
    return {};
}

DbErrorOr<void> MemoryBackedTable::drop_column(std::string const& column) {
    if (!get_column(column)) {
        return DbError { "Couldn't find column '" + column + "'" };
    }

    std::vector<Column> vec;

    for (size_t i = 0; i < m_columns.size(); i++) {
        if (m_columns[i].name() == column) {
            for (auto& row : m_rows) {
                row.remove(i);
            }
        }
        else {
            vec.push_back(std::move(m_columns[i]));
        }
    }

    m_columns = std::move(vec);

    return {};
}

DbErrorOr<void> MemoryBackedTable::perform_database_integrity_checks(Database& db, Tuple const& row) const {
    TRY(Table::perform_database_integrity_checks(db, row));

    Sql::AST::SelectColumns columns_to_context;
    {
        std::vector<Sql::AST::SelectColumns::Column> all_columns;
        for (auto const& column : columns()) {
            all_columns.push_back(Sql::AST::SelectColumns::Column { .column = std::make_unique<Sql::AST::Identifier>(0, column.name(), std::optional<std::string> {}) });
        }
        columns_to_context = Sql::AST::SelectColumns { std::move(all_columns) };
    }

    Sql::AST::EvaluationContext context { .db = &db };

    Sql::AST::SimpleTableExpression id { 0, *this };
    context.frames.emplace_back(&id, columns_to_context);

    // Checks and constraints
    if (check()) {
        if (check()->main_rule()) {
            context.current_frame().row = { .tuple = Tuple { row }, .source = {} };
            if (!TRY(TRY(check()->main_rule()->evaluate(context).map_error([](Sql::SQLError&& e) {
                    return DbError { e.message() };
                })).to_bool()))
                return DbError { "Values doesn't match general check rule specified for this table" };
        }

        for (const auto& expr : check()->constraints()) {
            context.current_frame().row = { .tuple = Tuple { row }, .source = {} };
            if (!TRY(TRY(expr.second->evaluate(context).map_error([](Sql::SQLError&& e) {
                    return DbError { e.message() };
                })).to_bool()))
                return DbError { "Values doesn't match '" + expr.first + "' check rule specified for this table" };
        }
    }
    return {};
}

DbErrorOr<void> MemoryBackedTable::insert_unchecked(Tuple const& row) {
    m_rows.push_back(row);
    return {};
}

void Table::export_to_csv(const std::string& path) const {
    std::ofstream f_out(path);

    unsigned i = 0;

    auto const& columns = this->columns();

    for (const auto& col : columns) {
        f_out << col.name();

        if (i < columns.size() - 1)
            f_out << ',';
        else
            f_out << '\n';
        i++;
    }

    rows().for_each_row([&](auto const& row) {
        i = 0;
        for (auto it = row.begin(); it != row.end(); it++) {
            f_out << it->to_string().release_value();

            if (i < columns.size() - 1)
                f_out << ',';
            else
                f_out << '\n';
            i++;
        }
    });
}

DbErrorOr<void> Table::import_from_csv(Database& db, Storage::CSVFile const& file) {
    for (auto const& row : file.rows()) {
        TRY(insert(db, row));
    }
    return {};
}

}
