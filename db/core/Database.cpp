#include "Database.hpp"
#include "db/core/Table.hpp"

namespace Db::Core {

Table& Database::create_table(std::string name) {
    return *m_tables.insert({ name, std::make_unique<MemoryBackedTable>() }).first->second;
}

DbErrorOr<void> Database::drop_table(std::string name) {
    TRY(table(name));

    m_tables.erase(name);
    return {};
}

DbErrorOr<Table*> Database::create_table_from_query(ResultSet select, std::string name) {
    return m_tables.insert({ name, TRY(MemoryBackedTable::create_from_select_result(select)) }).first->second.get();
}

DbErrorOr<Table*> Database::table(std::string name) {
    auto it = m_tables.find(name);
    if (it == m_tables.end()) {
        // TODO: Save location info
        return DbError { "Nonexistent table: " + name, 0 };
    }
    return it->second.get();
}

}
