#pragma once

#include <string>

namespace Db::Core {

class Column {
public:
    explicit Column(std::string name)
        : m_name(name) { }

    std::string name() const { return m_name; }

    std::string to_string() const { return m_name; }

private:
    std::string m_name;
};

}
