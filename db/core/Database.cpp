#include "Database.hpp"

namespace Db::Core {

Table& Database::create_table(std::string name) {
    return m_tables.emplace(name, Table()).first->second;
}

DbErrorOr<Table*> Database::table(std::string name) {
    auto it = m_tables.find(name);
    if (it == m_tables.end())
        return DbError { "Nonexistent table: " + name };
    return &it->second;
}

}
