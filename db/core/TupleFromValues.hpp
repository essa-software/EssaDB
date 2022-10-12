#pragma once

#include "Tuple.hpp"

namespace Db::Core {

class Database;
class Table;

DbErrorOr<Tuple> create_tuple_from_values(Database& db, Table& table, std::vector<std::pair<std::string, Value>> values);

}
