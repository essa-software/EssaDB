#include "Value.hpp"

#include "Row.hpp"
#include "core/SelectResult.hpp"

#include <ostream>

namespace Db::Core {

Value Value::null() {
    return Value { std::monostate {}, Type::Null };
}

Value Value::create_int(int i) {
    return Value { i, Type::Int };
}

Value Value::create_varchar(std::string s) {
    return Value { std::move(s), Type::Varchar };
}

Value Value::create_select_result(SelectResult result) {
    return Value { std::move(result), Type::SelectResult };
}

DbErrorOr<int> Value::to_int() const {
    switch (m_type) {
    // FIXME: Check in spec what should happen when converting null to 0
    case Type::Null:
        return 0;
    case Type::Int:
        return std::get<int>(*this);
    case Type::Varchar: {
        auto str = std::get<std::string>(*this);
        try {
            return std::stoi(str);
        } catch (...) {
            return DbError { "'" + str + "' is not a valid int" };
        }
    }
    case Type::SelectResult:
        return DbError { "SelectResult cannot be converted to int" };
    }
    __builtin_unreachable();
}

DbErrorOr<std::string> Value::to_string() const {
    switch (m_type) {
    case Type::Null:
        return "null";
    case Type::Int:
        return std::to_string(std::get<int>(*this));
    case Type::Varchar:
        return std::get<std::string>(*this);
    case Type::SelectResult:

        return DbError { "Select result is not a string" };
    }
    __builtin_unreachable();
}

std::string Value::to_debug_string() const {
    auto value = to_string().release_value();
    switch (m_type) {
    case Type::Null:
        return value;
    case Type::Int:
        return "int " + value;
    case Type::Varchar:
        return "varchar '" + value + "'";
    case Type::SelectResult:
        return "SelectResult (" + std::to_string(std::get<SelectResult>(*this).rows().size()) + " rows)";
    }
    __builtin_unreachable();
}

void Value::repl_dump(std::ostream& out) const {
    if (m_type == Type::SelectResult) {
        return std::get<SelectResult>(*this).dump(out);
    }
    else {
        out << to_debug_string() << std::endl;
    }
}

std::ostream& operator<<(std::ostream& out, Value const& value) {
    auto error = value.to_string();
    if (error.is_error())
        return out << "<invalid>";
    return out << error.release_value();
}

}
