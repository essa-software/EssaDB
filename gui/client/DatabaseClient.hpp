#pragma once

#include "gui/Structure.hpp"
#include <EssaGUI/gui/Container.hpp>
#include <db/core/ValueOrResultSet.hpp>
#include <db/sql/ast/Statement.hpp>
#include <memory>

namespace EssaDB {

class DatabaseClient;

class DatabaseClientType {
public:
    virtual ~DatabaseClientType() = default;
    virtual std::shared_ptr<GUI::Container> create_settings_widget() = 0;
    virtual Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> create(GUI::Container const* settings_widget) = 0;
    virtual Util::UString name() const = 0;
};

class DatabaseClient {
public:
    virtual ~DatabaseClient() = default;
    virtual Db::Core::DbErrorOr<Db::Core::ValueOrResultSet> run_query(std::string const& source) = 0;
    virtual Db::Core::DbErrorOr<Structure::Database> structure() const = 0;
    virtual Db::Core::DbErrorOr<void> import(std::string const& source, std::string const& table_name, Db::Core::ImportMode) = 0;
    virtual Util::UString status_string() const = 0;

    using TypeMap = std::unordered_map<std::string_view, std::unique_ptr<DatabaseClientType>>;

    static TypeMap const& types();
    static std::shared_ptr<GUI::Container> create_settings_widget(std::string_view);
    static Db::Core::DbErrorOr<std::unique_ptr<DatabaseClient>> create(std::string_view, GUI::Container const* settings_widget);
    static void register_type(std::string_view id, std::unique_ptr<DatabaseClientType>);
};

}
