#include "Database.hpp"

namespace Db::Core {

void Database::create_table(std::string name) {
    m_tables.emplace(name, Table());
}

DbErrorOr<Table*> Database::table(std::string name) {
    auto it = m_tables.find(name);
    if (it == m_tables.end())
        return DbError { "Nonexistent table: " + name };
    return &it->second;
}

}
