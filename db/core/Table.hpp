#pragma once

#include "AbstractTable.hpp"
#include "Column.hpp"
#include "DbError.hpp"
#include "ResultSet.hpp"
#include "RowWithColumnNames.hpp"

#include <db/util/NonCopyable.hpp>
#include <optional>
#include <set>

namespace Db::Core {

class Table : public Util::NonCopyable
    , public AbstractTable {
public:
    virtual DbErrorOr<void> truncate() = 0;
    virtual DbErrorOr<void> add_column(Column) = 0;
    virtual DbErrorOr<void> alter_column(Column) = 0;
    virtual DbErrorOr<void> drop_column(std::string const&) = 0;
    virtual DbErrorOr<void> insert(RowWithColumnNames::MapType) = 0;
    virtual int increment(std::string column) = 0;

    void export_to_csv(const std::string& path) const;
    DbErrorOr<void> import_from_csv(const std::string& path);
};

class MemoryBackedTable : public Table {
public:
    MemoryBackedTable() = default;

    static DbErrorOr<std::unique_ptr<MemoryBackedTable>> create_from_select_result(ResultSet const& select);

    virtual std::vector<Column> const& columns() const override { return m_columns; }

    virtual AbstractTableRowIterator<true> rows() const override {
        return { std::make_unique<MemoryBackedAbstractTableIteratorImpl<decltype(m_rows)::const_iterator>>(m_rows.begin(), m_rows.end()) };
    }

    virtual AbstractTableRowIterator<false> rows_writable() override {
        return { std::make_unique<WritableMemoryBackedAbstractTableIteratorImpl<decltype(m_rows)>>(m_rows) };
    }

    virtual size_t size() const override { return m_rows.size(); }

    std::vector<Tuple> const& raw_rows() const { return m_rows; }
    std::vector<Tuple>& raw_rows() { return m_rows; }

    virtual DbErrorOr<void> truncate() override {
        m_rows.clear();
        return {};
    }

    virtual DbErrorOr<void> add_column(Column) override;
    virtual DbErrorOr<void> alter_column(Column) override;
    virtual DbErrorOr<void> drop_column(std::string const&) override;
    virtual DbErrorOr<void> insert(RowWithColumnNames::MapType) override;

    struct CheckConstraint{
        // std::unique_ptr<AST::Expression> expr;
        std::optional<std::string> alias;
    };

private:
    friend class RowWithColumnNames;

    virtual int increment(std::string column) override { return ++m_auto_increment_values[column]; }

    std::vector<Tuple> m_rows;
    std::vector<Column> m_columns;
    std::map<std::string, int> m_auto_increment_values;
};

}
