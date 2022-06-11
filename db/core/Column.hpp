#pragma once

#include "Value.hpp"

#include <string>

namespace Db::Core {

class Column {
public:
    explicit Column(std::string name, Value::Type type)
        : m_name(name)
        , m_type(type) { }

    std::string name() const { return m_name; }
    Value::Type type() const { return m_type; }
    void set_type(Value::Type type){ m_type = type; }

    std::string to_string() const { return m_name; }


private:
    std::string m_name;
    Value::Type m_type {};
};

inline bool operator==(Column const& lhs, Column const& rhs){
    return lhs.name() == rhs.name() && lhs.type() == rhs.type();
}

}
