#pragma once

#include "Value.hpp"

#include <string>

namespace Db::Core {

class Column {
public:
    enum class AutoIncrement {
        Yes,
        No
    };

    explicit Column(std::string name, Value::Type type, AutoIncrement ai)
        : m_name(name)
        , m_type(type)
        , m_auto_increment(ai == AutoIncrement::Yes) { }

    std::string name() const { return m_name; }
    Value::Type type() const { return m_type; }
    bool auto_increment() const { return m_auto_increment; }
    void set_type(Value::Type type) { m_type = type; }

    std::string to_string() const { return m_name; }

private:
    std::string m_name;
    Value::Type m_type {};
    bool m_auto_increment {};
};

inline bool operator==(Column const& lhs, Column const& rhs) {
    return lhs.name() == rhs.name() && lhs.type() == rhs.type();
}

}
