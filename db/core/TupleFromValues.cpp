#include "TupleFromValues.hpp"

#include <db/core/Database.hpp>
#include <db/core/Table.hpp>

namespace Db::Core {

DbErrorOr<Tuple> create_tuple_from_values(Table& table, std::vector<std::pair<std::string, Value>> values) {
    std::vector<Value> row;
    auto& columns = table.columns();
    row.resize(columns.size());
    for (auto const& value : values) {
        auto column = table.get_column(value.first);
        if (!column) {
            // TODO: Save location info
            return DbError { "No such column in table: " + value.first, 0 };
        }
        row[column->index] = std::move(value.second);
    }

    return Tuple { row };
}
}
