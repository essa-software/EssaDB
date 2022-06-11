#pragma once

#include "Value.hpp"

#include <algorithm>
#include <span>
#include <utility>
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
    void remove(size_t index){
        std::vector<Value> vec;
        for(size_t i = 0; i < m_values.size(); i++){
            if(i != index)
                vec.push_back(std::move(m_values[i]));
        }

        m_values = std::move(vec);
    }

    void extend(){m_values.push_back(Value::null());}

    auto begin() const { return m_values.begin(); }
    auto end() const { return m_values.end(); }

private:
    std::vector<Value> m_values;
};

}
