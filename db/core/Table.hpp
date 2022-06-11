#pragma once

#include "Column.hpp"
#include "DbError.hpp"
#include "RowWithColumnNames.hpp"
#include "db/core/SelectResult.hpp"

#include <set>
#include <db/util/NonCopyable.hpp>

namespace Db::Core {

class Table : public Util::NonCopyable {
public:
    Table() = default;
    Table(SelectResult const& select);
    std::optional<std::pair<Column, size_t>> get_column(std::string const& name) const;
    size_t size() const { return m_rows.size(); }
    std::vector<Column> const& columns() const { return m_columns; }
    std::vector<Row> const& rows() const { return m_rows; }

    void truncate() {m_rows.clear();}
    void delete_row(size_t index);

    void export_to_csv(const std::string& path) const;
    DbErrorOr<void> import_from_csv(const std::string& path);

    DbErrorOr<void> add_column(Column);
    DbErrorOr<void> alter_column(Column);
    DbErrorOr<void> drop_column(Column);
    DbErrorOr<void> insert(RowWithColumnNames::MapType);

private:
    std::vector<Row> m_rows;
    std::vector<Column> m_columns;
};

}
