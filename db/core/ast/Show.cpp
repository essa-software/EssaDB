#include "Show.hpp"

#include <db/core/Database.hpp>
#include <db/core/ResultSet.hpp>

namespace Db::Core::AST {

DbErrorOr<ValueOrResultSet> Show::execute(Database& db) const {
    std::vector<Tuple> tuples;
    switch (m_type) {
    case Type::Tables: {
        db.for_each_table([&](auto const& table) {
            tuples.push_back(Tuple { Value::create_varchar(table.second->name()) });
        });
    } break;
    }
    return ResultSet { { "name" }, tuples };
}

}
