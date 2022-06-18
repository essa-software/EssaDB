#pragma once

#include "Column.hpp"
#include "DbError.hpp"
#include "RowWithColumnNames.hpp"
#include "AST.hpp"
#include "db/core/SelectResult.hpp"

#include <db/util/NonCopyable.hpp>
#include <set>

namespace Db::Core {

class Table : public Util::NonCopyable {
public:
    Table() = default;
    Table(SelectResult const& select);
    std::optional<std::pair<Column, size_t>> get_column(std::string const& name) const;
    size_t size() const { return m_rows.size(); }
    std::vector<Column> const& columns() const { return m_columns; }
    std::vector<Tuple> const& rows() const { return m_rows; }

    void truncate() { m_rows.clear(); }
    void delete_row(size_t index);

    void export_to_csv(const std::string& path) const;
    DbErrorOr<void> import_from_csv(const std::string& path);

    DbErrorOr<void> add_column(Column);
    DbErrorOr<void> update_cell(size_t row, size_t column, Value value);
    DbErrorOr<void> alter_column(Column);
    DbErrorOr<void> drop_column(Column);
    DbErrorOr<void> insert(RowWithColumnNames::MapType);

private:
    friend class RowWithColumnNames;

    auto increment(std::string column) { return ++m_auto_increment_values[column]; }

    std::vector<Tuple> m_rows;
    std::vector<Column> m_columns;
    std::map<std::string, int> m_auto_increment_values;
};

}
