#include "RowWithColumnNames.hpp"

#include "Table.hpp"
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

        if (column->first.type() != value.second.type() && value.second.type() != Value::Type::Null) {
            // TODO: Save location info
            return DbError { "Invalid value type for column '" + value.first + "': " + value.second.to_debug_string(), 0 };
        }

        if (column->first.unique()) {
            for(const auto& table_row : table.rows()){
                if(TRY(table_row.value(column->second) == value.second))
                    return DbError { "Not valid UNIQUE value.", 0 };
            }
        }else if (column->first.not_null()) {
            if(value.second.type() == Value::Type::Null)
                return DbError { "Value can't be null.", 0 };
        }

        row[column->second] = std::move(value.second);
    }

    // Null check
    for (size_t s = 0; s < row.size(); s++) {
        auto& value = row[s];
        if (value.type() == Value::Type::Null) {
            if (columns[s].auto_increment()) {
                if (columns[s].type() == Value::Type::Int)
                    value = Value::create_int(table.increment(columns[s].name()));
                else
                    return DbError { "Internal error: AUTO_INCREMENT used on non-int field", 0 };
            }else if(columns[s].not_null()) {
                if(value.type() == Value::Type::Null)
                    return DbError { "Value can't be null.", 0 };
            }else {
                value = columns[s].default_value();
            }
        }
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
