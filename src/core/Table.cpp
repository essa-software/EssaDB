#include "Table.hpp"

namespace Db::Core {

DbErrorOr<void> Table::add_column(Column column) {
    if (get_column(column.name()))
        return DbError { "Duplicate column '" + column.name() + "'" };
    m_columns.push_back(std::move(column));
    return {};
}

DbErrorOr<void> Table::insert(RowWithColumnNames::MapType map) {
    m_rows.push_back(TRY(RowWithColumnNames::from_map(*this, map)).row());
    return {};
}

std::optional<std::pair<Column, size_t>> Table::get_column(std::string const& name) const {
    size_t index = 0;
    for (auto& column : m_columns) {
        if (column.name() == name)
            return { { column, index } };
        index++;
    }
    return {};
}

DbErrorOr<SelectResult> Table::select(Query query) const {

    auto should_include_column = [&](std::string const& name) {
        return query.select_all || query.columns.contains(name);
    };

    auto should_include_row = [&](Row const& row) -> DbErrorOr<bool> {
        if (!query.filter)
            return true;
        return TRY(query.filter->is_true(row.value(get_column(query.filter->column)->second)));
    };

    std::vector<Row> rows;
    for (auto const& row : m_rows) {
        // WHERE
        if (!TRY(should_include_row(row)))
            continue;
        std::vector<Value> values;
        size_t index = 0;
        for (auto const& value : row) {
            // SELECT
            if (should_include_column(m_columns[index].name()))
                values.push_back(value);
            index++;
        }
        rows.push_back(Row { values });
    }

    std::vector<std::string> column_names;
    for (auto const& column : m_columns) {
        if (should_include_column(column.name()))
            column_names.push_back(column.to_string());
    }

    return SelectResult { column_names, std::move(rows) };
}

}
