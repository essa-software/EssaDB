#pragma once

#include <EssaUtil/NonCopyable.hpp>
#include <db/core/DbError.hpp>
#include <db/core/ImportMode.hpp>
#include <db/core/Table.hpp>
#include <string>
#include <unordered_map>

namespace Db::Core {

class Database : public Util::NonCopyable {
public:
    // Create a new MemoryBackedTable.
    Table& create_table(std::string name, std::shared_ptr<Sql::AST::Check> check);

    // Move already created table into a database.
    void add_table(std::unique_ptr<Table> table);
    
    // Create a new MemoryBackedTable initialized by the given ResultSet.
    DbErrorOr<Table*> create_table_from_query(ResultSet select, std::string name);

    DbErrorOr<void> drop_table(std::string name);
    DbErrorOr<Table*> table(std::string name);

    bool exists(std::string name) const { return m_tables.find(name) != m_tables.end(); }

    DbErrorOr<void> import_to_table(std::string const& path, std::string const& table_name, ImportMode);

    size_t table_count() const { return m_tables.size(); }

    template<class Callback>
    void for_each_table(Callback&& c) const {
        for (auto const& table : m_tables)
            c(table);
    }

    void remove_all_tables() {
        m_tables.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Table>> m_tables;
};

}
