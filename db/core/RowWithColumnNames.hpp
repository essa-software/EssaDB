#pragma once

#include "DbError.hpp"
#include "Row.hpp"

#include <map>
#include <string>

namespace Db::Core {

class Table;

class RowWithColumnNames {
public:
    using MapType = std::map<std::string, Value>;

    static DbErrorOr<RowWithColumnNames> from_map(Table const& table, MapType map);

    RowWithColumnNames(Row row, Table const& table)
        : m_row(std::move(row))
        , m_table(table) { }

    Row row() const { return m_row; }

    friend std::ostream& operator<<(std::ostream& out, RowWithColumnNames const& row);

private:
    Row m_row;
    Table const& m_table;
};

}
