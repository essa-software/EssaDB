#include "Database.hpp"
#include "db/core/Table.hpp"
#include <EssaUtil/Config.hpp>

namespace Db::Core {

Table& Database::create_table(std::string name, std::shared_ptr<AST::Check> check) {
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
        // TODO: Save location info
        return DbError { "Nonexistent table: " + name, 0 };
    }
    return it->second.get();
}

DbErrorOr<void> Database::import_to_table(std::string const& path, std::string const& table_name, AST::Import::Mode mode) {
    auto& new_table = create_table(table_name, std::make_shared<AST::Check>(0));
    switch (mode) {
    case AST::Import::Mode::Csv:
        TRY(new_table.import_from_csv(path));
        break;
    default:
        ESSA_UNREACHABLE;
    }
    return {};
}

}
