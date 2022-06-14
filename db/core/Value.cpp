#include "Value.hpp"

#include "SelectResult.hpp"
#include "Tuple.hpp"

#include <cctype>
#include <ostream>
#include <sstream>

namespace Db::Core {

Value::Type find_type(const std::string& str) {
    if (str == "null")
        return Value::Type::Null;

    for (const auto& c : str) {
        if (c < '0' || c > '9')
            return Value::Type::Varchar;
    }

    return Value::Type::Int;
}

Value Value::null() {
    return Value { std::monostate {}, Type::Null };
}

Value Value::create_int(int i) {
    return Value { i, Type::Int };
}

Value Value::create_varchar(std::string s) {
    return Value { std::move(s), Type::Varchar };
}

Value Value::create_bool(bool b) {
    return Value { b, Type::Bool };
}
Value Value::create_time(Util::Clock::time_point t) {
    return Value { t, Type::Time };
}

Value Value::create_time(std::string time, Util::Clock::Format format) {
    // FIXME: Add some ate format checking ASAP
    switch (format) {
    case Util::Clock::Format::NO_CLOCK_AMERICAN: {
        size_t year = 0, month = 0, day = 0, counter = 0;
        std::string str = "";

        for (const auto& t : time) {
            if (t == '-') {
                if (counter == 0)
                    year = std::stoi(str);
                else if (counter == 1)
                    month = std::stoi(str);
                else if (counter == 2)
                    day = std::stoi(str);
                else
                    break;
                str = "";
                counter++;
            }
            else {
                if (isdigit(t))
                    str += t;
                else
                    return Value::null();
            }
        }

        if (!str.empty())
            day = std::stoi(str);
        return Value(Util::Time::create(year, month, day), Type::Time);
    }
    default:
        return Value::null();
    }
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
            // TODO: Save location info
            return DbError { "'" + str + "' is not a valid int", 0 };
        }
    }
    case Type::Bool:
        return std::get<bool>(*this) ? 1 : 0;
    case Type::Time:
        return static_cast<int>(std::get<Util::Clock::time_point>(*this).time_since_epoch().count());
    case Type::SelectResult: {
        auto select_result = std::get<SelectResult>(*this);
        if (select_result.rows().size() != 1)
            return DbError { "SelectResult must have only one 1 row to be convertible to int", 0 };
        if (select_result.rows()[0].value_count() != 1)
            return DbError { "SelectResult must have only one 1 column to be convertible to int", 0 };
        return TRY(select_result.rows()[0].value(0).to_int());
    }
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
    case Type::Bool:
        return std::get<bool>(*this) ? "true" : "false";
    case Type::Time: {
        Util::time_format = Util::Clock::Format::NO_CLOCK_AMERICAN;
        auto time_point = std::get<Util::Clock::time_point>(*this);
        std::ostringstream stream;
        stream << time_point;
        return stream.str();
    }
    case Type::SelectResult: {
        auto select_result = std::get<SelectResult>(*this);
        if (select_result.rows().size() != 1)
            return DbError { "SelectResult must have only one 1 row to be convertible to string", 0 };
        if (select_result.rows()[0].value_count() != 1)
            return DbError { "SelectResult must have only one 1 column to be convertible to string", 0 };
        return TRY(select_result.rows()[0].value(0).to_string());
    }
    }
    __builtin_unreachable();
}

DbErrorOr<bool> Value::to_bool() const {
    return TRY(to_int()) != 0;
}

DbErrorOr<SelectResult> Value::to_select_result() const {
    if (m_type != Type::SelectResult) {
        // TODO: Save location info
        return DbError { "Value '" + to_debug_string() + "' is not a select result", 0 };
    }
    return std::get<SelectResult>(*this);
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
    case Type::Bool:
        return "bool " + value;
    case Type::Time:
        return "time " + value;
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
