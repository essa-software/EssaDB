#pragma once

#include <db/core/Column.hpp>
#include <string>
#include <vector>

namespace Db::Core {

struct TableSetup {
    std::string name;
    std::vector<Core::Column> columns;
};

}
