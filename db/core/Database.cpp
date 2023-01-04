#include "Database.hpp"
#include "db/core/TupleFromValues.hpp"

#include <EssaUtil/Config.hpp>
#include <db/core/Table.hpp>
#include <db/storage/CSVFile.hpp>
#include <db/storage/FileBackedTable.hpp>
#include <filesystem>

namespace Db::Core {

Core::DbErrorOr<Table*> Database::create_table(TableSetup table_setup, std::shared_ptr<Sql::AST::Check> check, DatabaseEngine engine) {
    // FIXME: Implement CREATE TABLE IF NOT EXISTS.
    if (m_tables.contains(table_setup.name)) {
        return Core::DbError { fmt::format("Table '{}' already exists", table_setup.name) };
    }

    switch (engine) {
    case DatabaseEngine::Memory: {
        auto& table = *m_tables.insert({ table_setup.name, std::make_unique<MemoryBackedTable>(std::move(check), table_setup) }).first->second;
        return &table;
    }
    case DatabaseEngine::EDB: {
        if (!std::filesystem::is_directory("database")) {
            std::filesystem::create_directory("database");
        }
        auto filename = fmt::format("database/{}.edb", table_setup.name);
        auto result = Storage::FileBackedTable::initialize(filename, table_setup);
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

DbErrorOr<void> Database::restructure_table(std::string const& old_name, TableSetup const& table_setup) {
    if (old_name != table_setup.name && m_tables.contains(table_setup.name)) {
        return Core::DbError { fmt::format("Table '{}' already exists", table_setup.name) };
    }

    // 1. Rename the old table to some "backup" name.
    // FIXME: This hopes that table of this name doesn't
    //        exist, this may not be the case!
    auto backup_name = old_name + "__BACKUP" + std::to_string(time(nullptr));

    auto it = m_tables.find(old_name);
    auto backup_table = m_tables.insert({ backup_name, std::move(it->second) }).first->second.get();
    m_tables.erase(it);
    TRY(backup_table->rename(backup_name));

    // 2. Create a new table
    auto check = [&]() -> std::shared_ptr<Sql::AST::Check> {
        auto memory_backed_table = dynamic_cast<MemoryBackedTable*>(backup_table);
        if (!memory_backed_table) {
            return nullptr;
        }
        return memory_backed_table->check();
    }();
    auto new_table = TRY(create_table(table_setup, check, backup_table->engine()));

    // 3. Copy all the rows to the new table
    TRY(backup_table->rows().try_for_each_row([&](auto const& row) -> DbErrorOr<void> {
        std::vector<std::pair<std::string, Value>> new_tuple;
        size_t s = 0;
        for (auto const& value : row) {
            auto column_name = backup_table->columns()[s].name();
            if (new_table->get_column(column_name)) {
                new_tuple.push_back({ column_name, value });
            }
            s++;
        }
        TRY(new_table->insert(this, TRY(create_tuple_from_values(*new_table, std::move(new_tuple)))));
        return {};
    }));

    // 4. Drop "backup" table.
    // FIXME: Actually drop data that this table contains (for EDB)
    m_tables.erase(backup_name);

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

DbErrorOr<Table*> Database::import_to_table(std::string const& path, std::string const& table_name, ImportMode mode, DatabaseEngine engine) {
    switch (mode) {
    case ImportMode::Csv: {
        Table* table = m_tables.contains(table_name)
            ? MUST(this->table(table_name))
            : nullptr;
        auto csv_file = TRY(Storage::CSVFile::import(path, table ? table->columns() : std::vector<Column> {}));
        if (!table) {
            table = TRY(create_table({ table_name, csv_file.columns() }, nullptr, engine));
        }
        TRY(table->import_from_csv(this, csv_file));
        return table;
    }
    }
    ESSA_UNREACHABLE;
}

}
