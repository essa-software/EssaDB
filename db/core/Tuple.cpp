#include "Tuple.hpp"
#include "Column.hpp"

namespace Db::Core {

Column::Column(std::string name, Value::Type type, bool ai, bool un, bool nn, std::optional<Value> def_val)
    : m_name(name)
    , m_type(type)
    , m_auto_increment(ai)
    , m_unique(un)
    , m_not_null(nn)
    , m_default_value(std::move(def_val)) {}

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

std::ostream& operator<<(std::ostream& out, Tuple const& tuple) {
    out << "(";
    size_t index = 0;
    for (auto const& value : tuple) {
        out << value;
        if (index != tuple.m_values.size() - 1)
            return out << ", ";
        index++;
    }
    return out << ")";
}

}
