#include "Database.hpp"

namespace Db::Core {

Table& Database::create_table(std::string name) {
    return m_tables.emplace(name, Table()).first->second;
}

Table& Database::create_table_from_query(SelectResult select, std::string name){
    return m_tables.emplace(name, Table(select)).first->second;
}

DbErrorOr<Table*> Database::table(std::string name) {
    auto it = m_tables.find(name);
    if (it == m_tables.end()) {
        // TODO: Save location info
        return DbError { "Nonexistent table: " + name, 0 };
    }
    return &it->second;
}

}
