#pragma once

#include <db/core/Value.hpp>
#include <string>
#include <vector>

namespace EssaDB::Structure {

struct Column {
    std::string name;
    Db::Core::Value::Type type;
};
struct Table {
    std::string name;
    std::vector<Column> columns;
};
struct Database {
    std::vector<Table> tables;
};

}
