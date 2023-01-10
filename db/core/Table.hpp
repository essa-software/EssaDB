#pragma once

#include <EssaUtil/NonCopyable.hpp>
#include <db/core/Column.hpp>
#include <db/core/DatabaseEngine.hpp>
#include <db/core/DbError.hpp>
#include <db/core/IndexedRelation.hpp>
#include <db/core/ResultSet.hpp>
#include <db/core/TableSetup.hpp>
#include <db/storage/CSVFile.hpp>
#include <map>
#include <memory>
#include <optional>
#include <set>

// FIXME: Get rid of Core -> SQL dependency
#include <db/sql/ast/Expression.hpp>

namespace Db::Core {

class Table : public Util::NonCopyable
    , public IndexedRelation {
public:
    virtual DatabaseEngine engine() const = 0;
    virtual std::string name() const = 0;
    virtual int next_auto_increment_value(std::string const& column) = 0;
    virtual int increment(std::string const& column) = 0;
    virtual DbErrorOr<void> rename(std::string const& new_name) = 0;

    DbErrorOr<void> insert(Database* db, Tuple const&);

    // NOTE: This doesn't check types and integrity in any way!
    virtual DbErrorOr<void> insert_unchecked(Tuple const&) = 0;

    void export_to_csv(const std::string& path) const;
    DbErrorOr<void> import_from_csv(Database* db, Storage::CSVFile const& file);

protected:
    // Check integrity with database, i.e foreign keys, checks, constraints, ...
    virtual DbErrorOr<void> perform_database_integrity_checks(Database* db, Tuple const& row) const;

private:
    DbErrorOr<void> check_value_validity(Tuple const& row, size_t column_index) const;

    // Check integrity with table, i.e if types match, if columns are NON NULL/UNIQUE, primary keys, ...
    DbErrorOr<void> perform_table_integrity_checks(Tuple const& row) const;
};

class MemoryBackedTable : public Table {
public:
    auto begin() { return m_rows.data(); }
    auto end() { return m_rows.data() + m_rows.size(); }

    MemoryBackedTable(std::shared_ptr<Sql::AST::Check> check, TableSetup const& setup)
        : m_columns(setup.columns)
        , m_check(std::move(check))
        , m_name(setup.name) { }

    static DbErrorOr<std::unique_ptr<MemoryBackedTable>> create_from_select_result(ResultSet const& select);

    virtual DatabaseEngine engine() const override { return DatabaseEngine::Memory; }
    virtual std::vector<Column> const& columns() const override { return m_columns; }

    virtual RelationIterator rows() const override {
        return RelationIterator { std::make_unique<MemoryBackedRelationIteratorImpl<decltype(m_rows)::const_iterator>>(m_rows.begin(), m_rows.end()) };
    }
    virtual MutableRelationIterator writable_rows() override {
        return MutableRelationIterator { std::make_unique<MemoryBackedRelationIteratorImpl<decltype(m_rows)::iterator>>(m_rows.begin(), m_rows.end()) };
    }

    virtual size_t size() const override { return m_rows.size(); }

    std::vector<Tuple> const& raw_rows() const { return m_rows; }
    std::vector<Tuple>& raw_rows() { return m_rows; }
    virtual std::string name() const override { return m_name; }

    std::shared_ptr<Sql::AST::Check>& check() { return m_check; }
    std::shared_ptr<Sql::AST::Check> const& check() const { return m_check; }

    virtual DbErrorOr<void> insert_unchecked(Tuple const&) override;

private:
    virtual int next_auto_increment_value(std::string const& column) override { return m_auto_increment_values[column] + 1; }
    virtual int increment(std::string const& column) override { return ++m_auto_increment_values[column]; }
    virtual DbErrorOr<void> rename(std::string const& new_name) override;
    virtual DbErrorOr<void> perform_database_integrity_checks(Database* db, Tuple const& row) const override;

    std::vector<Tuple> m_rows;
    std::vector<Column> m_columns;
    std::shared_ptr<Sql::AST::Check> m_check;
    std::map<std::string, int> m_auto_increment_values;
    std::string m_name;
};

}
