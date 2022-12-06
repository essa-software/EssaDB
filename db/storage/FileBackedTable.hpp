#pragma once

#include <EssaUtil/Error.hpp>
#include <db/core/Table.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <db/storage/edb/EDBFile.hpp>

namespace Db::Storage {

class FileBackedTable : public Core::Table {
public:
    static Util::OsErrorOr<std::unique_ptr<FileBackedTable>> initialize(std::string const&, EDB::EDBFile::TableSetup);
    static Util::OsErrorOr<std::unique_ptr<FileBackedTable>> open(std::string const&);

    // ^Relation
    virtual std::vector<Core::Column> const& columns() const override;
    virtual Core::RelationIterator rows() const override;
    virtual size_t size() const override;

    // ^Table
    virtual std::string name() const override;
    virtual Core::DbErrorOr<void> truncate() override;
    virtual Core::DbErrorOr<void> add_column(Core::Column) override;
    virtual Core::DbErrorOr<void> alter_column(Core::Column) override;
    virtual Core::DbErrorOr<void> drop_column(std::string const&) override;
    virtual int next_auto_increment_value(std::string const& column) override;
    virtual int increment(std::string const& column) override;
    virtual Core::DbErrorOr<void> insert_unchecked(Core::Tuple const&) override;

private:
    friend std::unique_ptr<FileBackedTable> std::make_unique<FileBackedTable>(std::unique_ptr<Db::Storage::EDB::EDBFile>&&);

    FileBackedTable(std::unique_ptr<EDB::EDBFile>);
    static Util::OsErrorOr<std::unique_ptr<FileBackedTable>> create(std::unique_ptr<EDB::EDBFile> file);
    Util::OsErrorOr<void> read_header();

    std::unique_ptr<EDB::EDBFile> m_file;
    std::vector<Core::Column> m_columns;
};

}
