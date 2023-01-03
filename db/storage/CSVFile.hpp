#pragma once

#include <db/core/Column.hpp>
#include <db/core/DbError.hpp>
#include <db/core/Tuple.hpp>
#include <string>
#include <vector>

namespace Db::Storage {

class CSVFile {
public:
    // Empty column hint means automatic type deduction. Otherwise, types
    // must match the given column hint.
    static Core::DbErrorOr<CSVFile> import(std::string const& path, std::vector<Core::Column> const& column_hint);

    std::vector<Core::Column> const& columns() const { return m_columns; }
    std::vector<Core::Tuple> const& rows() const { return m_rows; }

private:
    Core::DbErrorOr<void> do_import(std::string const& path, std::vector<Core::Column> const& column_hint);

    std::vector<Core::Column> m_columns;
    std::vector<Core::Tuple> m_rows;
};

}
