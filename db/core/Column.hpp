#pragma once

#include "Tuple.hpp"
#include "Value.hpp"

#include <optional>
#include <string>

namespace Db::Core {
    class Table;

class Column {
public:
    explicit Column(std::string name, Value::Type type, bool ai, bool un, bool nn, std::optional<Value> def_val = {});

    std::string name() const { return m_name; }
    Value::Type type() const { return m_type; }
    Value default_value() const { return m_default_value ? *m_default_value : Value::null(); }

    bool auto_increment() const { return m_auto_increment; }
    bool unique() const { return m_unique; }
    bool not_null() const { return m_not_null; }

    void set_type(Value::Type type) { m_type = type; }

    Table* const& original_table() const { return m_original_table; }
    void set_table(Table* table) { m_original_table = table; }

    std::string to_string() const { return m_name; }

private:
    std::string m_name;
    Table* m_original_table = nullptr;
    Value::Type m_type {};
    bool m_auto_increment {};
    bool m_unique {};
    bool m_not_null {};

    std::optional<Value> m_default_value = {};
};

inline bool operator==(Column const& lhs, Column const& rhs) {
    return lhs.name() == rhs.name() && lhs.type() == rhs.type();
}

}
