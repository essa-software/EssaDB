#include "RowWithColumnNames.hpp"

#include "Table.hpp"
#include "db/core/Expression.hpp"
#include "db/core/Tuple.hpp"
#include "db/core/Value.hpp"

#include <ostream>

namespace Db::Core {

DbErrorOr<RowWithColumnNames> RowWithColumnNames::from_map(Table& table, MapType map) {
    std::vector<Value> row;
    auto& columns = table.columns();
    row.resize(columns.size());
    for (auto& value : map) {
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
            if (value.second.type() == Value::Type::Null)
                return DbError { "Value can't be null.", 0 };
        }

        row[column->index] = std::move(value.second);
    }
    AST::SelectColumns select_all_columns;

    AST::SelectColumns const& columns_to_context = *TRY([&table, &select_all_columns]() -> DbErrorOr<AST::SelectColumns const*> {
        std::vector<AST::SelectColumns::Column> all_columns;
        for (auto const& column : table.columns()) {
            all_columns.push_back(AST::SelectColumns::Column { .column = std::make_unique<AST::Identifier>(0, column.name(), std::optional<std::string>{}) });
        }
        select_all_columns = AST::SelectColumns { std::move(all_columns) };
        return &select_all_columns;
    }());
    
    AST::EvaluationContext context{ .columns = columns_to_context, .table = &table, .row_type = AST::EvaluationContext::RowType::FromTable};

    // Null check
    for (size_t s = 0; s < row.size(); s++) {
        auto& value = row[s];
        if (value.type() == Value::Type::Null) {
            if (columns[s].auto_increment()) {
                if (columns[s].type() == Value::Type::Int)
                    value = Value::create_int(table.increment(columns[s].name()));
                else
                    return DbError { "Internal error: AUTO_INCREMENT used on non-int field", 0 };
            }
            else if (columns[s].not_null()) {
                if (value.type() == Value::Type::Null)
                    return DbError { "Value can't be null.", 0 };
            }
            else {
                value = columns[s].default_value();
            }
        }
    }

    auto memory_backed_table = dynamic_cast<MemoryBackedTable*>(&table);

    if(!memory_backed_table)
        return DbError { "Table with invalid type!", 0 };

    if(memory_backed_table->check_value()){
        if(!TRY(TRY(memory_backed_table->check_value()->evaluate(context, AST::TupleWithSource{.tuple = Tuple{row}, .source = {}})).to_bool()))
            return DbError { "Values doesn't match general check rule specified for this table", 0 };
    }

    for(const auto& expr : memory_backed_table->check_map()){
        if(!TRY(TRY(expr.second->evaluate(context, AST::TupleWithSource{.tuple = Tuple{row}, .source = {}})).to_bool()))
            return DbError { "Values doesn't match '" + expr.first +"' check rule specified for this table", 0 };
    }

    return RowWithColumnNames { Tuple { row }, table };
}

std::ostream& operator<<(std::ostream& out, RowWithColumnNames const& row) {
    size_t index = 0;
    out << "{ ";
    for (auto& value : row.m_row) {
        auto column = row.m_table.columns()[index];
        out << column.name() << ": " << value;
        if (index != row.m_row.value_count() - 1)
            out << ", ";
        index++;
    }
    return out << " }";
}

}
