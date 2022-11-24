#include "EssaDBDatabaseClient.hpp"

#include "gui/Structure.hpp"
#include <db/sql/SQL.hpp>

namespace EssaDB {

Db::Core::DbErrorOr<Db::Core::ValueOrResultSet> EssaDBDatabaseClient::run_query(std::string const& source) {
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
    return m_db.import_to_table(source, table_name, mode);
}

Util::UString EssaDBDatabaseClient::status_string() const {
    return "EssaDB";
}

std::shared_ptr<GUI::Container> EssaDBDatabaseClientType::create_settings_widget() {
    return nullptr;
}

Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> EssaDBDatabaseClientType::create(GUI::Container const*) {
    return std::make_unique<EssaDBDatabaseClient>();
}

}
