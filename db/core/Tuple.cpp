#include "Tuple.hpp"

namespace Db::Core {

bool operator<(Tuple const& lhs, Tuple const& rhs) {
    for (size_t s = 0; s < std::max(lhs.value_count(), rhs.value_count()); s++) {
        auto const& lhs_value = s >= lhs.value_count() ? Value::null() : lhs.value(s);
        auto const& rhs_value = s >= rhs.value_count() ? Value::null() : rhs.value(s);

        auto result = lhs_value == rhs_value;
        if (result.is_error()) {
            // TODO: Propagate errors
            return false;
        }
        if (result.release_value())
            continue;

        result = lhs_value < rhs_value;
        if (result.is_error()) {
            // TODO: Propagate errors
            return false;
        }
        return result.release_value();
    }
    return false;
}

}
