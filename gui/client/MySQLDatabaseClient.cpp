#include "MySQLDatabaseClient.hpp"
#include "mysql.h"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/ScopeGuard.hpp>
#include <gui/Structure.hpp>
#include <gui/client/ConnectToMySQLDialog.hpp>

namespace EssaDB {

MySQLDatabaseClient::MySQLDatabaseClient(MySQLConnectionData data)
    : m_connection_data(std::move(data)) { }

MySQLDatabaseClient::~MySQLDatabaseClient() {
    mysql_close(m_mysql_connection);
}

Db::Core::DbErrorOr<void> MySQLDatabaseClient::connect() {
    m_mysql_connection = mysql_init(NULL);
    if (m_mysql_connection == NULL) {
        return Db::Core::DbError { fmt::format("Failed to initialize: {}", mysql_error(m_mysql_connection)), 0 };
    }

    if (!mysql_real_connect(m_mysql_connection,
            m_connection_data.address.data(),
            m_connection_data.username.data(),
            m_connection_data.password.data(),
            m_connection_data.database.data(),
            m_connection_data.port, NULL, 0)) {

        return Db::Core::DbError { fmt::format("Failed to connect to server: {}", mysql_error(m_mysql_connection)), 0 };
    }
    return {};
}

Db::Core::DbErrorOr<Db::Core::ValueOrResultSet> MySQLDatabaseClient::run_query(std::string const& query) {
    MYSQL_RES* result;
    MYSQL_FIELD* field;
    MYSQL_ROW row;

    if (mysql_query(m_mysql_connection, query.data())) {
        return Db::Core::DbError { fmt::format("Error querying server: {}", mysql_error(m_mysql_connection)), 0 };
    }

    result = mysql_use_result(m_mysql_connection);
    if (result == NULL) {
        return Db::Core::Value::null();
    }

    Util::ScopeGuard guard { [&]() {
        mysql_free_result(result);
    } };

    std::vector<std::string> column_names;
    std::vector<Db::Core::Tuple> rows;

    int num_fields = mysql_num_fields(result);

    while ((field = mysql_fetch_field(result))) {
        column_names.push_back(field->name);
    }

    while ((row = mysql_fetch_row(result))) {
        std::vector<Db::Core::Value> values;
        for (int i = 0; i < num_fields; i++) {
            std::string str = row[i] ? row[i] : "NULL";
            values.push_back(Db::Core::Value::create_varchar(std::move(str)));
        }
        rows.push_back(Db::Core::Tuple { values });
    }

    return Db::Core::ResultSet { column_names, rows };
}

Db::Core::DbErrorOr<Structure::Database> MySQLDatabaseClient::structure() const {
    MYSQL_RES* result;
    MYSQL_ROW row;

    if (mysql_query(m_mysql_connection, ("SELECT * FROM information_schema.columns WHERE table_schema LIKE '" + m_connection_data.database + "' ORDER BY table_name;").data())) {
        return Db::Core::DbError { fmt::format("Error querying table schema: {}", mysql_error(m_mysql_connection)), 0 };
    }

    result = mysql_use_result(m_mysql_connection);
    Util::ScopeGuard result_guard { [&]() {
        mysql_free_result(result);
    } };

    Structure::Database database;

    std::set<std::string> varchar_set { "blob", "char", "longblob", "longtext", "mediumtext", "set", "text", "varbinary", "varchar" };
    std::set<std::string> int_set { "int", "smallint", "tinyint", "enum" };
    std::set<std::string> float_set { "decimal", "double", "float" };
    std::set<std::string> date_set { "datetime", "time", "timestamp" };

    Structure::Table* table = nullptr;
    std::string last_table = "";
    while ((row = mysql_fetch_row(result))) {
        std::string table_name = row[2];

        if (last_table != table_name) {
            table = &database.tables.emplace_back();
            table->name = table_name;
            last_table = table_name;
        }

        assert(table);
        std::string column_name = row[3];
        std::string type_str = row[7];
        Db::Core::Value::Type type;

        if (varchar_set.contains(type_str)) {
            type = Db::Core::Value::Type::Varchar;
        }
        else if (int_set.contains(type_str)) {
            type = Db::Core::Value::Type::Int;
        }
        else if (float_set.contains(type_str)) {
            type = Db::Core::Value::Type::Float;
        }
        else if (date_set.contains(type_str)) {
            type = Db::Core::Value::Type::Time;
        }
        else {
            type = Db::Core::Value::Type::Bool;
        }

        table->columns.push_back(Structure::Column { .name = std::move(column_name), .type = type });
    }

    return database;
}

Db::Core::DbErrorOr<void> MySQLDatabaseClient::import(std::string const& filename, std::string const& table_name, Db::Core::AST::Import::Mode mode) {
    assert(mode == Db::Core::AST::Import::Mode::Csv);

    // FIXME: Requiring a database here is a hack, fix that!
    Db::Core::Database db;
    Db::Core::MemoryBackedTable table { nullptr, table_name };
    TRY(table.import_from_csv(db, filename));

    // TODO: Escape
    std::string create_query = "CREATE TABLE `" + table_name + "`(";
    bool first = true;

    for (const auto& column : table.columns()) {
        if (!first) {
            create_query += ", ";
        }

        create_query += "`" + column.name() + "` ";
        switch (column.type()) {
        case Db::Core::Value::Type::Int:
            create_query += "int";
            break;
        case Db::Core::Value::Type::Float:
            create_query += "float";
            break;
        case Db::Core::Value::Type::Varchar:
            create_query += "varchar(255)";
            break;
        case Db::Core::Value::Type::Bool:
            create_query += "int";
            break;
        case Db::Core::Value::Type::Time:
            create_query += "varchar(255)";
            break;
        case Db::Core::Value::Type::Null:
            ESSA_UNREACHABLE;
        }

        first = false;
    }

    create_query += ");";

    std::cout << create_query << "\n";

    if (mysql_query(m_mysql_connection, create_query.data())) {
        return Db::Core::DbError { fmt::format("Error querying server: {}", mysql_error(m_mysql_connection)), 0 };
    }

    // TODO: Actually insert the data

    return {};
}

Util::UString MySQLDatabaseClient::status_string() const {
    return Util::UString { "MySQL: " + m_connection_data.database + "@" + m_connection_data.address + ":" + std::to_string(m_connection_data.port) };
}

std::shared_ptr<GUI::Container> MySQLDatabaseClientType::create_settings_widget() {
    return std::make_unique<ConnectToMySQLDialog>();
}

Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> MySQLDatabaseClientType::create(GUI::Container const* settings_widget) {
    auto data = TRY(static_cast<ConnectToMySQLDialog const*>(settings_widget)->connection_data());
    auto connection = std::make_unique<MySQLDatabaseClient>(data);
    TRY(connection->connect());
    return connection;
}

}
