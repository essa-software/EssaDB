#include "Database.hpp"

#include <EssaUtil/Config.hpp>
#include <db/core/Table.hpp>

namespace Db::Core {

Table& Database::create_table(std::string name, std::shared_ptr<Sql::AST::Check> check) {
    return *m_tables.insert({ name, std::make_unique<MemoryBackedTable>(std::move(check), name) }).first->second;
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
        return DbError { "Nonexistent table: " + name };
    }
    return it->second.get();
}

DbErrorOr<void> Database::import_to_table(std::string const& path, std::string const& table_name, ImportMode mode) {
    auto& new_table = create_table(table_name, std::make_shared<Sql::AST::Check>(0));
    switch (mode) {
    case ImportMode::Csv:
        TRY(new_table.import_from_csv(*this, path));
        break;
    default:
        ESSA_UNREACHABLE;
    }
    return {};
}

}
