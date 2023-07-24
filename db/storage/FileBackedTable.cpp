#include "FileBackedTable.hpp"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/Error.hpp>
#include <db/core/Column.hpp>
#include <db/core/Relation.hpp>
#include <db/storage/edb/EDBRelationIterator.hpp>
#include <fcntl.h>

namespace Db::Storage {

Util::OsErrorOr<std::unique_ptr<FileBackedTable>> FileBackedTable::initialize(std::string database_path, Core::TableSetup setup) {
    auto path = fmt::format("{}/{}.edb", database_path, setup.name);
    Util::File file { ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644), true };
    auto table = TRY(FileBackedTable::create(TRY(EDB::EDBFile::initialize(std::move(file), setup))));
    table->m_database_path = std::move(database_path);
    table->m_table_name = std::move(setup.name);
    return table;
}

Util::OsErrorOr<std::unique_ptr<FileBackedTable>> FileBackedTable::open(std::string database_path, std::string table_name) {
    auto path = fmt::format("{}/{}.edb", database_path, table_name);
    Util::File file { ::open(path.c_str(), O_RDWR), true };
    auto table = TRY(FileBackedTable::create(TRY(EDB::EDBFile::open(std::move(file)))));
    table->m_database_path = std::move(database_path);
    table->m_table_name = std::move(table_name);
    return table;
}

FileBackedTable::FileBackedTable(std::unique_ptr<EDB::EDBFile> file)
    : m_file(std::move(file)) {
}

Util::OsErrorOr<std::unique_ptr<FileBackedTable>> FileBackedTable::create(std::unique_ptr<EDB::EDBFile> file) {
    auto table = std::make_unique<FileBackedTable>(std::move(file));
    TRY(table->read_header());
    return table;
}

Util::OsErrorOr<void> FileBackedTable::read_header() {
    m_columns = TRY(m_file->read_columns());
    return {};
}

std::vector<Core::Column> const& FileBackedTable::columns() const {
    return m_columns;
}

Core::RelationIterator FileBackedTable::rows() const {
    return Core::RelationIterator { std::make_unique<EDB::EDBRelationIteratorImpl>(*m_file) };
}

Core::MutableRelationIterator FileBackedTable::writable_rows() {
    return Core::MutableRelationIterator { std::make_unique<EDB::EDBRelationIteratorImpl>(*m_file) };
}

size_t FileBackedTable::size() const {
    return m_file->header().row_count;
}

std::string FileBackedTable::name() const {
    return m_file->read_heap(m_file->header().table_name).decode_infallible().encode();
}

int FileBackedTable::next_auto_increment_value(std::string const&) {
    // TODO
    return 0;
}

int FileBackedTable::increment(std::string const&) {
    // TODO
    return 0;
}

Core::DbError os_to_db_error(Util::OsError&& error) {
    return Core::DbError { fmt::format("OSError: {}: {}", error.function, strerror(error.error)) };
};

Core::DbErrorOr<void> FileBackedTable::rename(std::string const& new_name) {
    // 1. Update header
    TRY(m_file->rename(new_name).map_error(os_to_db_error));

    // 2. Actually move the file.
    auto old_edb_file_path = edb_file_path();
    m_table_name = new_name;
    if (::rename(old_edb_file_path.c_str(), edb_file_path().c_str()) < 0) {
        return Core::DbError { fmt::format("File rename failed: {}", strerror(errno)) };
    }
    return {};
}

Core::DbErrorOr<void> FileBackedTable::insert_unchecked(Core::Tuple const& tuple) {
    TRY(m_file->insert(tuple).map_error(os_to_db_error));
    return {};
}

void FileBackedTable::dump_storage_debug() {
    fmt::print("path={}\n", m_database_path);
    m_file->dump();
}

std::string FileBackedTable::edb_file_path() const {
    return fmt::format("{}/{}.edb", m_database_path, m_table_name);
}

}
