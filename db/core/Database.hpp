#pragma once

#include "Table.hpp"

#include <string>
#include <unordered_map>
#include <db/util/NonCopyable.hpp>

namespace Db::Core {

class Database : public Util::NonCopyable {
public:
    Table& create_table(std::string name);
    Table& create_table_from_query(SelectResult select, std::string name);
    DbErrorOr<Table*> table(std::string name);

private:
    std::unordered_map<std::string, Table> m_tables;
};

}
