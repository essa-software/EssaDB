#pragma once

#include "Value.hpp"

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace Db::Core {

class Tuple {
public:
    Tuple(std::initializer_list<Value> values)
        : m_values(values.size()) {
        std::copy(values.begin(), values.end(), m_values.begin());
    }

    Tuple(std::span<Value> values)
        : m_values(values.size()) {
        std::copy(values.begin(), values.end(), m_values.begin());
    }

    size_t value_count() const { return m_values.size(); }
    Value value(size_t index) const { return m_values[index]; }
    void set_value(size_t index, Value value) { m_values[index] = std::move(value); }
    void remove(size_t index) {
        std::vector<Value> vec;
        for (size_t i = 0; i < m_values.size(); i++) {
            if (i != index)
                vec.push_back(std::move(m_values[i]));
        }

        m_values = std::move(vec);
    }

    void extend() { m_values.push_back(Value::null()); }

    auto begin() const { return m_values.begin(); }
    auto end() const { return m_values.end(); }

    void clear_row() { m_values.clear(); }

private:
    friend std::ostream& operator<<(std::ostream&, Tuple const&);

    std::vector<Value> m_values;
};

inline DbErrorOr<bool> operator==(Tuple const& lhs, Tuple const& rhs) {
    if (lhs.value_count() != rhs.value_count())
        return false;
    for (auto it1 = lhs.begin(), it2 = rhs.begin(); it1 != lhs.end(); it1++, it2++) {
        if (!TRY(*it1 == *it2))
            return false;
    }
    return true;
}

bool operator<(Tuple const& lhs, Tuple const& rhs);

}
