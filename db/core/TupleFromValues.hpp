#pragma once

#include "Tuple.hpp"

namespace Db::Core {

class Table;

// Form a contiguous tuple from columns and their names as given
// in INSERT. This doesn't perform integrity checks.
DbErrorOr<Tuple> create_tuple_from_values(Table& table, std::vector<std::pair<std::string, Value>> values);

}
