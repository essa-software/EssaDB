#pragma once

#include "Table.hpp"

#include <string>
#include <unordered_map>
#include <util/NonCopyable.hpp>

namespace Db::Core {

class Database : public Util::NonCopyable {
public:
    void create_table(std::string name);
    DbErrorOr<Table*> table(std::string name);

private:
    std::unordered_map<std::string, Table> m_tables;
};

}
