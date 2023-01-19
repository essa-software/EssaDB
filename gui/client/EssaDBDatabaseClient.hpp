#pragma once

#include "DatabaseClient.hpp"
#include <db/core/Database.hpp>

namespace EssaDB {

class EssaDBDatabaseClient : public DatabaseClient {
public:
    virtual Db::Sql::SQLErrorOr<Db::Core::ValueOrResultSet> run_query(std::string const& source) override;
    virtual Db::Core::DbErrorOr<Structure::Database> structure() const override;
    virtual Db::Core::DbErrorOr<void> import(std::string const& source, std::string const& table_name, Db::Core::ImportMode) override;
    virtual Util::UString status_string() const override;

private:
    Db::Core::Database m_db = Db::Core::Database::create_memory_backed();
};

class EssaDBDatabaseClientType : public DatabaseClientType {
public:
    virtual std::shared_ptr<GUI::Container> create_settings_widget() override;
    virtual Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> create(GUI::Container const* settings_widget) override;
    virtual Util::UString name() const override { return "EssaDB"; }
};

}
