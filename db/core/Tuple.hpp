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

    Tuple(std::vector<Value> values)
        : m_values(std::move(values)) {
    }

    size_t value_count() const { return m_values.size(); }
    Value value(size_t index) const {
        assert(index < m_values.size());
        return m_values[index];
    }
    void set_value(size_t index, Value value) {
        assert(index < m_values.size());
        m_values[index] = std::move(value);
    }
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

struct TupleWithSource {
    Tuple tuple;
    std::optional<Tuple> source;

    DbErrorOr<bool> operator==(TupleWithSource const& other) const {
        return TRY(tuple == other.tuple) && source.has_value() == other.source.has_value() && TRY(source.value() == other.source.value());
    }
};

}

template<>
class fmt::formatter<Db::Core::Tuple> : public fmt::formatter<std::string_view> {
public:
    auto format(Db::Core::Tuple const& tuple, fmt::format_context& ctx) const {
        fmt::format_to(ctx.out(), "(");
        for (size_t s = 0; s < tuple.value_count(); s++) {
            fmt::format_to(ctx.out(), "{}", tuple.value(s).to_debug_string());
            if (s != tuple.value_count() - 1) {
                fmt::format_to(ctx.out(), ", ");
            }
        }
        fmt::format_to(ctx.out(), ")");
        return ctx.out();
    }
};
