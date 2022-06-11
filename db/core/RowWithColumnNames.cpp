#include "RowWithColumnNames.hpp"

#include "Table.hpp"

#include <ostream>

namespace Db::Core {

DbErrorOr<RowWithColumnNames> RowWithColumnNames::from_map(Table const& table, MapType map) {
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
        row[column->second] = std::move(value.second);
    }
    // TODO: Null check
    return RowWithColumnNames { Row { row }, table };
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
