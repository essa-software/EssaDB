#include "EssaDBDatabaseClient.hpp"

#include <db/core/Database.hpp>
#include <db/sql/SQL.hpp>
#include <gui/Structure.hpp>
#include <gui/client/ConnectToEssaDBDialog.hpp>

namespace EssaDB {

Util::OsErrorOr<std::unique_ptr<EssaDBDatabaseClient>> EssaDBDatabaseClient::create(std::optional<std::string> path) {
    auto db = path
        ? TRY(Db::Core::Database::create_or_open_file_backed(*path))
        : Db::Core::Database::create_memory_backed();
    return std::unique_ptr<EssaDBDatabaseClient>(new EssaDBDatabaseClient(std::move(db)));
}

Db::Sql::SQLErrorOr<Db::Core::ValueOrResultSet> EssaDBDatabaseClient::run_query(std::string const& source) {
    return Db::Sql::run_query(m_db, source);
}

Db::Core::DbErrorOr<Structure::Database> EssaDBDatabaseClient::structure() const {
    Structure::Database db;
    m_db.for_each_table([&db](auto const& table) {
        std::vector<Structure::Column> columns;
        for (auto const& column : table.second->columns()) {
            columns.push_back(Structure::Column { .name = column.name(), .type = column.type() });
        }
        db.tables.push_back(Structure::Table { .name = table.first, .columns = columns });
    });
    return db;
}

Db::Core::DbErrorOr<void> EssaDBDatabaseClient::import(std::string const& source, std::string const& table_name, Db::Core::ImportMode mode) {
    TRY(m_db.import_to_table(source, table_name, mode, Db::Core::DatabaseEngine::Memory));
    return {};
}

Util::UString EssaDBDatabaseClient::status_string() const {
    return "EssaDB";
}

std::shared_ptr<GUI::Container> EssaDBDatabaseClientType::create_settings_widget() {
    return std::make_unique<ConnectToEssaDBDialog>();
}

Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> EssaDBDatabaseClientType::create(GUI::Container const* settings_widget) {
    auto data = static_cast<ConnectToEssaDBDialog const*>(settings_widget)->connection_data();
    auto connection = TRY(EssaDBDatabaseClient::create(data.path).map_error([](Util::OsError const& error) {
        return Db::Core::DbError { fmt::format("Opening database failed: {}", error) };
    }));
    return connection;
}

}
