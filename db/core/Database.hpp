#pragma once

#include "AST.hpp"
#include "DbError.hpp"
#include "Table.hpp"

#include <EssaUtil/NonCopyable.hpp>
#include <string>
#include <unordered_map>

namespace Db::Core {

class Database : public Util::NonCopyable {
public:
    Table& create_table(std::string name, std::shared_ptr<AST::Check> check);
    DbErrorOr<Table*> create_table_from_query(ResultSet select, std::string name);

    DbErrorOr<void> drop_table(std::string name);
    DbErrorOr<Table*> table(std::string name);

    bool exists(std::string name) const { return m_tables.find(name) != m_tables.end(); }

    DbErrorOr<void> import_to_table(std::string const& path, std::string const& table_name, AST::Import::Mode);

    size_t table_count() const { return m_tables.size(); }

    template<class Callback>
    void for_each_table(Callback&& c) const {
        for (auto const& table : m_tables)
            c(table);
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Table>> m_tables;
};

}
