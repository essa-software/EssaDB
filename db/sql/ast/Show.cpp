#include "Show.hpp"

#include <db/core/Database.hpp>
#include <db/core/ResultSet.hpp>

namespace Db::Sql::AST {

SQLErrorOr<Core::ValueOrResultSet> Show::execute(Core::Database& db) const {
    std::vector<Core::Tuple> tuples;
    switch (m_type) {
    case Type::Tables: {
        db.for_each_table([&](auto const& table) {
            tuples.push_back(Core::Tuple { Core::Value::create_varchar(table.second->name()) });
        });
    } break;
    }
    return Core::ResultSet { { "name" }, tuples };
}

}
