#include "TupleFromValues.hpp"

#include <db/core/Database.hpp>
#include <db/core/Table.hpp>
#include <db/core/ast/TableExpression.hpp>

namespace Db::Core {

DbErrorOr<Tuple> create_tuple_from_values(Database& db, Table& table, std::vector<std::pair<std::string, Value>> values) {
    std::vector<Value> row;
    auto& columns = table.columns();
    row.resize(columns.size());
    for (auto const& value : values) {
        auto column = table.get_column(value.first);
        if (!column) {
            // TODO: Save location info
            return DbError { "No such column in table: " + value.first, 0 };
        }

        if (column->column.type() != value.second.type() && value.second.type() != Value::Type::Null) {
            // TODO: Save location info
            return DbError { "Invalid value type for column '" + value.first + "': " + value.second.to_debug_string(), 0 };
        }

        if (column->column.unique()) {
            TRY(table.rows().try_for_each_row([&](Tuple const& row) -> DbErrorOr<void> {
                if (TRY(row.value(column->index) == value.second))
                    return DbError { "Not valid UNIQUE value.", 0 };
                return {};
            }));
        }
        else if (column->column.not_null()) {
            if (value.second.is_null())
                return DbError { "Value can't be null.", 0 };
        }

        row[column->index] = std::move(value.second);
    }

    AST::SelectColumns columns_to_context;
    {
        std::vector<AST::SelectColumns::Column> all_columns;
        for (auto const& column : table.columns()) {
            all_columns.push_back(AST::SelectColumns::Column { .column = std::make_unique<AST::Identifier>(0, column.name(), std::optional<std::string> {}) });
        }
        columns_to_context = AST::SelectColumns { std::move(all_columns) };
    }

    AST::EvaluationContext context { .db = &db };

    AST::SimpleTableExpression id { 0, table };
    context.frames.emplace_back(&id, columns_to_context);

    // Null check
    for (size_t s = 0; s < row.size(); s++) {
        auto& value = row[s];
        if (value.is_null()) {
            if (columns[s].auto_increment()) {
                if (columns[s].type() == Value::Type::Int)
                    value = Value::create_int(table.increment(columns[s].name()));
                else
                    return DbError { "Internal error: AUTO_INCREMENT used on non-int field", 0 };
            }
            else if (columns[s].not_null()) {
                if (value.is_null())
                    return DbError { "Value can't be null.", 0 };
            }
            else {
                value = columns[s].default_value();
            }
        }
    }

    // Primary key (NULL + duplicate check)
    // For now, this is checked even if no constraint is explicitly defined.
    if (table.primary_key()) {
        auto const& pk = table.primary_key();
        auto column = table.get_column(pk->local_column);
        if (!column) {
            return DbError { fmt::format("Internal error: Nonexistent column '{}' used as primary key", pk->local_column), 0 };
        }
        auto const& value = row[column->index];
        if (value.is_null()) {
            return DbError { "Primary key may not be null", 0 };
        }
        TRY(table.rows().try_for_each_row([&](Tuple const& row) -> DbErrorOr<void> {
            if (TRY(row.value(column->index) == value))
                return DbError { "Primary key must be unique", 0 };
            return {};
        }));
    }

    // Foreign keys (row existence in the foreign table)
    // For now, this is checked even if no constraint is explicitly defined.
    for (auto const& fk : table.foreign_keys()) {
        auto local_column = table.get_column(fk.local_column);
        if (!local_column) {
            return DbError { fmt::format("Internal error: Nonexistent column '{}' used as foreign key", fk.local_column), 0 };
        }
        auto referenced_table = TRY(db.table(fk.referenced_table));
        auto referenced_column = referenced_table->get_column(fk.referenced_column);
        if (!referenced_column) {
            return DbError { fmt::format("Internal error: Nonexistent column '{}' used as referenced table in foreign key", fk.referenced_column), 0 };
        }

        auto const& local_value = row[local_column->index];
        if (local_value.is_null()) {
            continue;
        }

        if (!referenced_table->find_first_matching_tuple(referenced_column->index, local_value)) {
            return DbError { fmt::format("Foreign key '{}' requires matching value in referenced column '{}.{}'",
                                 fk.local_column, fk.referenced_table, fk.referenced_column),
                0 };
        }
    }

    // Checks and constraints
    auto memory_backed_table = dynamic_cast<MemoryBackedTable*>(&table);

    if (!memory_backed_table)
        return DbError { "Table with invalid type!", 0 };

    if (memory_backed_table->check()) {
        if (memory_backed_table->check()->main_rule()) {
            context.current_frame().row = { .tuple = Tuple { row }, .source = {} };
            if (!TRY(TRY(memory_backed_table->check()->main_rule()->evaluate(context)).to_bool()))
                return DbError { "Values doesn't match general check rule specified for this table", 0 };
        }

        for (const auto& expr : memory_backed_table->check()->constraints()) {
            context.current_frame().row = { .tuple = Tuple { row }, .source = {} };
            if (!TRY(TRY(expr.second->evaluate(context)).to_bool()))
                return DbError { "Values doesn't match '" + expr.first + "' check rule specified for this table", 0 };
        }
    }

    return Tuple { row };
}
}
