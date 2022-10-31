#include "DatabaseClient.hpp"

namespace EssaDB {

static DatabaseClient::TypeMap s_database_client_types;

DatabaseClient::TypeMap const& DatabaseClient::types() {
    return s_database_client_types;
}

std::shared_ptr<GUI::Container> DatabaseClient::create_settings_widget(std::string_view id) {
    return s_database_client_types[id]->create_settings_widget();
}

Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> DatabaseClient::create(std::string_view id, GUI::Container const* settings_widget) {
    return s_database_client_types[id]->create(settings_widget);
}

void DatabaseClient::register_type(std::string_view id, std::unique_ptr<DatabaseClientType> type) {
    s_database_client_types.insert({ id, std::move(type) });
}

}
