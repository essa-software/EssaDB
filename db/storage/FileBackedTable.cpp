#include "FileBackedTable.hpp"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/Error.hpp>
#include <db/core/Column.hpp>
#include <db/core/Relation.hpp>
#include <db/storage/edb/EDBRelationIterator.hpp>

namespace Db::Storage {

Util::OsErrorOr<std::unique_ptr<FileBackedTable>> FileBackedTable::initialize(std::string const& filename, Core::TableSetup setup) {
    return FileBackedTable::create(TRY(EDB::EDBFile::initialize(filename, std::move(setup))), filename);
}

Util::OsErrorOr<std::unique_ptr<FileBackedTable>> FileBackedTable::open(std::string const& filename) {
    return FileBackedTable::create(TRY(EDB::EDBFile::open(filename)), filename);
}

FileBackedTable::FileBackedTable(std::unique_ptr<EDB::EDBFile> file)
    : m_file(std::move(file)) {
}

Util::OsErrorOr<std::unique_ptr<FileBackedTable>> FileBackedTable::create(std::unique_ptr<EDB::EDBFile> file, std::string const& filename) {
    auto table = std::make_unique<FileBackedTable>(std::move(file));
    table->m_file_path = filename;
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

size_t FileBackedTable::size() const {
    return m_file->header().row_count;
}

std::string FileBackedTable::name() const {
    return m_file->read_heap(m_file->header().table_name).decode().encode();
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
    // FIXME: This breaks abstraction, as FileBackedTable may come from anywhere
    //        and this assumes it comes from Database. Make this handled in
    //        Database OR make "database/{}.eml" be handled here.
    std::filesystem::path path = m_file_path;
    path.replace_filename(new_name + ".edb");
    if (::rename(m_file_path.c_str(), path.c_str()) < 0) {
        return Core::DbError { fmt::format("File rename failed: {}", strerror(errno)) };
    }
    return {};
}

Core::DbErrorOr<void> FileBackedTable::insert_unchecked(Core::Tuple const& tuple) {
    TRY(m_file->insert(tuple).map_error(os_to_db_error));
    return {};
}

}
