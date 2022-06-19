#include "Value.hpp"

#include "SelectResult.hpp"
#include "Tuple.hpp"
#include "db/core/DbError.hpp"
#include "db/util/Clock.hpp"

#include <cctype>
#include <chrono>
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

DbErrorOr<Value> Value::from_string(Type type, std::string const& string) {
    switch (type) {
    case Type::Null:
        return DbError { "Internal error: trying to parse string into null type", 0 };
    case Type::Int:
        try {
            return Value::create_int(std::stoi(string));
        } catch (...) {
            return DbError { "Cannot convert '" + string + "' to int", 0 };
        }
    case Type::Float:
        try {
            return Value::create_float(std::stof(string));
        } catch (...) {
            return DbError { "Cannot convert '" + string + "' to float", 0 };
        }
    case Type::Varchar:
        return Value::create_varchar(string);
    case Type::Bool:
        if (string == "true")
            return Value::create_bool(true);
        else if (string == "false")
            return Value::create_bool(false);
        return DbError { "Bool value must be either 'true' or 'false', got '" + string + "'", 0 };
    case Type::Time:
        return Value::create_time(string, Util::Clock::Format::NO_CLOCK_AMERICAN);
    case Type::SelectResult:
        return DbError { "Internal error: TODO: Parse string into SelectResult", 0 };
    }
    __builtin_unreachable();
}

Value Value::null() {
    return Value { std::monostate {}, Type::Null };
}

Value Value::create_int(int i) {
    return Value { i, Type::Int };
}

Value Value::create_float(float f) {
    return Value { f, Type::Float };
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
    case Type::Float:
        return static_cast<int>(std::get<float>(*this));
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

DbErrorOr<float> Value::to_float() const {
    switch (m_type) {
    // FIXME: Check in spec what should happen when converting null to 0
    case Type::Null:
        return 0.f;
    case Type::Int:
        return static_cast<float>(std::get<int>(*this));
    case Type::Float:
        return std::get<float>(*this);
    case Type::Varchar: {
        auto str = std::get<std::string>(*this);
        try {
            return std::stof(str);
        } catch (...) {
            // TODO: Save location info
            return DbError { "'" + str + "' is not a valid float", 0 };
        }
    }
    case Type::Bool:
        return std::get<bool>(*this) ? 1.f : 0.f;
    case Type::Time:
        return DbError { "Time is not convertible to float", 0 };
    case Type::SelectResult: {
        auto select_result = std::get<SelectResult>(*this);
        if (select_result.rows().size() != 1)
            return DbError { "SelectResult must have only one 1 row to be convertible to float", 0 };
        if (select_result.rows()[0].value_count() != 1)
            return DbError { "SelectResult must have only one 1 column to be convertible to float", 0 };
        return TRY(select_result.rows()[0].value(0).to_float());
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
    case Type::Float:
        return std::to_string(std::get<float>(*this));
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
    case Type::Float:
        return "float " + value;
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

DbErrorOr<Value> operator+(Value const& lhs, Value const& rhs) {
    switch (lhs.type()) {
    case Value::Type::Bool:
        return Value::create_bool(TRY(lhs.to_bool()) + TRY(rhs.to_bool()));
    case Value::Type::Int:
        return Value::create_int(TRY(lhs.to_int()) + TRY(rhs.to_int()));
    case Value::Type::Float:
        return Value::create_float(TRY(lhs.to_float()) + TRY(rhs.to_float()));
    case Value::Type::Null:
        return Value::null();
    case Value::Type::Varchar:
        return Value::create_varchar(TRY(lhs.to_string()) + TRY(rhs.to_string()));
    case Value::Type::Time: {
        time_t time = TRY(lhs.to_int()) + TRY(rhs.to_int());
        Util::Clock::duration dur(time);
        return Value::create_time(Util::Clock::time_point(dur));
    }
    case Value::Type::SelectResult:
        return DbError { "No matching operator '+' for 'SelectResult' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<Value> operator-(Value const& lhs, Value const& rhs) {
    switch (lhs.type()) {
    case Value::Type::Bool:
        return Value::create_bool(TRY(lhs.to_bool()) - TRY(rhs.to_bool()));
    case Value::Type::Int:
        return Value::create_int(TRY(lhs.to_int()) - TRY(rhs.to_int()));
    case Value::Type::Float:
        return Value::create_float(TRY(lhs.to_float()) - TRY(rhs.to_float()));
    case Value::Type::Null:
        return Value::null();
    case Value::Type::Varchar:
        return DbError { "No matching operator '-' for 'VARCHAR' type.", 0 };
    case Value::Type::Time: {
        time_t time = TRY(lhs.to_int()) - TRY(rhs.to_int());
        Util::Clock::duration dur(time);
        return Value::create_time(Util::Clock::time_point(dur));
    }
    case Value::Type::SelectResult:
        return DbError { "No matching operator '-' for 'SelectResult' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<Value> operator*(Value const& lhs, Value const& rhs) {
    switch (lhs.type()) {
    case Value::Type::Bool:
        // FIXME: Is this legal?
        return Value::create_bool(TRY(lhs.to_bool()) && TRY(rhs.to_bool()));
    case Value::Type::Int:
        return Value::create_int(TRY(lhs.to_int()) * TRY(rhs.to_int()));
    case Value::Type::Float:
        return Value::create_float(TRY(lhs.to_float()) * TRY(rhs.to_float()));
    case Value::Type::Null:
        return Value::null();
    case Value::Type::Varchar:
        return DbError { "No matching operator '*' for 'VARCHAR' type.", 0 };
    case Value::Type::Time:
        return DbError { "No matching operator '*' for 'TIME' type.", 0 };
    case Value::Type::SelectResult:
        return DbError { "No matching operator '*' for 'SelectResult' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<Value> operator/(Value const& lhs, Value const& rhs) {
    switch (lhs.type()) {
    case Value::Type::Bool:
        return Value::create_bool(TRY(lhs.to_bool()) / TRY(rhs.to_bool()));
    case Value::Type::Int:
        return Value::create_int(TRY(lhs.to_int()) / TRY(rhs.to_int()));
    case Value::Type::Float:
        return Value::create_float(TRY(lhs.to_float()) / TRY(rhs.to_float()));
    case Value::Type::Null:
        return Value::null();
    case Value::Type::Varchar:
        return DbError { "No matching operator '/' for 'VARCHAR' type.", 0 };
    case Value::Type::Time:
        return DbError { "No matching operator '/' for 'TIME' type.", 0 };
    case Value::Type::SelectResult:
        return DbError { "No matching operator '/' for 'SelectResult' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<bool> operator<(Value const& lhs, Value const& rhs) {
    if (lhs.type() == Value::Type::Null)
        return true;
    if (rhs.type() == Value::Type::Null)
        return false;

    switch (lhs.type()) {
    case Value::Type::Bool:
        return TRY(lhs.to_bool()) < TRY(rhs.to_bool());
    case Value::Type::Int:
        return TRY(lhs.to_int()) < TRY(rhs.to_int());
    case Value::Type::Float:
        return TRY(lhs.to_float()) < TRY(rhs.to_float());
    case Value::Type::Null:
        return TRY(lhs.to_int()) < TRY(rhs.to_int());
    case Value::Type::Varchar:
        return TRY(lhs.to_string()) < TRY(rhs.to_string());
    case Value::Type::Time: {
        return TRY(lhs.to_int()) < TRY(rhs.to_int());
    }
    case Value::Type::SelectResult:
        return DbError { "No matching operator '<' for 'SelectResult' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<bool> operator<=(Value const& lhs, Value const& rhs) {
    return TRY(lhs < rhs) || TRY(lhs == rhs);
}

DbErrorOr<bool> operator==(Value const& lhs, Value const& rhs) {
    switch (lhs.type()) {
    case Value::Type::Bool:
        return TRY(lhs.to_bool()) == TRY(rhs.to_bool());
    case Value::Type::Int:
        return TRY(lhs.to_int()) == TRY(rhs.to_int());
    case Value::Type::Float:
        return TRY(lhs.to_float()) == TRY(rhs.to_float());
    case Value::Type::Null:
        return rhs.type() == Value::Type::Null;
    case Value::Type::Varchar:
        return TRY(lhs.to_string()) == TRY(rhs.to_string());
    case Value::Type::Time: {
        return TRY(lhs.to_int()) == TRY(rhs.to_int());
    }
    case Value::Type::SelectResult:
        return DbError { "No matching operator '==' for 'SelectResult' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<bool> operator>=(Value const& lhs, Value const& rhs) {
    return !TRY(lhs < rhs);
}

DbErrorOr<bool> operator>(Value const& lhs, Value const& rhs) {
    return !TRY(lhs < rhs) && !TRY(lhs == rhs);
}

DbErrorOr<bool> operator!=(Value const& lhs, Value const& rhs) {
    return !TRY(lhs == rhs);
}

}
