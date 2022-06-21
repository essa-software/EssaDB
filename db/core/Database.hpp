#pragma once

#include "Table.hpp"
#include "db/core/DbError.hpp"

#include <db/util/NonCopyable.hpp>
#include <string>
#include <unordered_map>

namespace Db::Core {

class Database : public Util::NonCopyable {
public:
    Table& create_table(std::string name, std::shared_ptr<AST::Expression> check, std::map<std::string, std::shared_ptr<AST::Expression>> check_map);
    DbErrorOr<Table*> create_table_from_query(ResultSet select, std::string name);

    DbErrorOr<void> drop_table(std::string name);
    DbErrorOr<Table*> table(std::string name);

    bool exists(std::string name) const { return m_tables.find(name) != m_tables.end(); }

private:
    std::unordered_map<std::string, std::unique_ptr<Table>> m_tables;
};

}
