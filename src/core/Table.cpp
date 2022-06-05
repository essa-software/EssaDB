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

SelectResult Table::select() const {
    std::vector<std::string> column_names;
    for (auto const& column : m_columns)
        column_names.push_back(column.to_string());

    std::vector<Row> rows = m_rows;

    return SelectResult { column_names, std::move(rows) };
}

}
