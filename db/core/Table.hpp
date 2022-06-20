#pragma once

#include "AbstractTable.hpp"
#include "Column.hpp"
#include "DbError.hpp"
#include "ResultSet.hpp"
#include "RowWithColumnNames.hpp"

#include <db/util/NonCopyable.hpp>
#include <set>

namespace Db::Core {

class Table : public Util::NonCopyable
    , public AbstractTable {
public:
    Table() = default;

    static DbErrorOr<Table> create_from_select_result(ResultSet const& select);

    virtual std::vector<Column> const& columns() const override { return m_columns; }
    virtual AbstractTableRowIterator rows() const override {
        return { std::make_unique<MemoryBackedAbstractTableIteratorImpl<decltype(m_rows)::const_iterator>>(m_rows.begin(), m_rows.end()) };
    }

    std::vector<Tuple> const& raw_rows() const { return m_rows; }
    std::vector<Tuple>& raw_rows() { return m_rows; }

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
