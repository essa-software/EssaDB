#include "Database.hpp"

#include <EssaUtil/Config.hpp>
#include <db/core/Table.hpp>
#include <db/storage/CSVFile.hpp>
#include <db/storage/FileBackedTable.hpp>
#include <filesystem>

namespace Db::Core {

Core::DbErrorOr<Table*> Database::create_table(TableSetup table_setup, std::shared_ptr<Sql::AST::Check> check, Engine engine) {
    switch (engine) {
    case Engine::Memory: {
        auto& table = *m_tables.insert({ table_setup.name, std::make_unique<MemoryBackedTable>(std::move(check), table_setup.name) }).first->second;
        for (auto const& column : table_setup.columns) {
            TRY(table.add_column(column));
        }
        return &table;
    }
    case Engine::EDB: {
        if (!std::filesystem::is_directory("database")) {
            std::filesystem::create_directory("database");
        }
        auto filename = fmt::format("database/{}.edb", table_setup.name);
        auto result = Storage::FileBackedTable::initialize(filename, std::move(table_setup));
        if (result.is_error()) {
            return Core::DbError { fmt::format("Creating table failed: {}", result.release_error()) };
        }
        return &*m_tables.insert({ table_setup.name, result.release_value() }).first->second;
    } break;
    }
    ESSA_UNREACHABLE;
}

void Database::add_table(std::unique_ptr<Table> table) {
    m_tables.insert({ table->name(), std::move(table) });
}

DbErrorOr<void> Database::drop_table(std::string name) {
    TRY(table(name));

    m_tables.erase(name);
    return {};
}

DbErrorOr<Table*> Database::create_table_from_query(ResultSet select, std::string name) {
    return m_tables.insert({ name, TRY(MemoryBackedTable::create_from_select_result(select)) }).first->second.get();
}

DbErrorOr<Table*> Database::table(std::string name) {
    auto it = m_tables.find(name);
    if (it == m_tables.end()) {
        return DbError { "Nonexistent table: " + name };
    }
    return it->second.get();
}

DbErrorOr<Table*> Database::import_to_table(std::string const& path, std::string const& table_name, ImportMode mode, Engine engine) {
    switch (mode) {
    case ImportMode::Csv: {
        Table* table = m_tables.contains(table_name)
            ? MUST(this->table(table_name))
            : nullptr;
        auto csv_file = TRY(Storage::CSVFile::import(path, table ? table->columns() : std::vector<Column> {}));
        if (!table) {
            table = TRY(create_table({ table_name, csv_file.columns() }, nullptr, engine));
        }
        TRY(table->import_from_csv(*this, csv_file));
        return table;
    }
    }
    ESSA_UNREACHABLE;
}

}
