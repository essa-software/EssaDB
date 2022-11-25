#pragma once

#include "ConnectToMySQLDialog.hpp"
#include "DatabaseClient.hpp"
#include "mysql.h"
#include <db/core/Database.hpp>

namespace EssaDB {

class MySQLDatabaseClient : public DatabaseClient {
public:
    MySQLDatabaseClient(MySQLConnectionData);
    virtual ~MySQLDatabaseClient();

    Db::Core::DbErrorOr<void> connect();
    virtual Db::Sql::SQLErrorOr<Db::Core::ValueOrResultSet> run_query(std::string const& source) override;
    virtual Db::Core::DbErrorOr<Structure::Database> structure() const override;
    virtual Db::Core::DbErrorOr<void> import(std::string const& source, std::string const& table_name, Db::Core::ImportMode) override;
    virtual Util::UString status_string() const override;

private:
    Db::Sql::SQLErrorOr<Db::Core::ValueOrResultSet> do_run_query(std::string const& source) const;
    Db::Sql::SQLErrorOr<std::string> get_current_database() const;

    MYSQL* m_mysql_connection = nullptr;
    MySQLConnectionData m_connection_data;
};

class MySQLDatabaseClientType : public DatabaseClientType {
public:
    virtual std::shared_ptr<GUI::Container> create_settings_widget() override;
    virtual Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> create(GUI::Container const* settings_widget) override;
    virtual Util::UString name() const override { return "MySQL"; }
};

}
