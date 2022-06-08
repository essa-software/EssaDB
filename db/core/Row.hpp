#pragma once

#include "Value.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace Db::Core {

class Row {
public:
    Row(std::span<Value> values)
        : m_values(values.size()) {
        std::copy(values.begin(), values.end(), m_values.begin());
    }

    size_t value_count() const { return m_values.size(); }
    Value value(size_t index) const { return m_values[index]; }

    auto begin() const { return m_values.begin(); }
    auto end() const { return m_values.end(); }

private:
    std::vector<Value> m_values;
};

}
