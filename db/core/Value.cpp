#include "Value.hpp"

#include "DbError.hpp"
#include "ResultSet.hpp"
#include "Tuple.hpp"
#include <EssaUtil/SimulationClock.hpp>

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
        return Value::create_time(string, Util::SimulationClock::Format::NO_CLOCK_AMERICAN);
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

Value Value::create_time(Util::SimulationClock::time_point t) {
    return Value { t, Type::Time };
}

Value Value::create_time(std::string time, Util::SimulationClock::Format format) {
    // FIXME: Add some ate format checking ASAP
    switch (format) {
    case Util::SimulationClock::Format::NO_CLOCK_AMERICAN: {
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
        return Value(Util::SimulationTime::create(year, month, day), Type::Time);
    }
    default:
        return Value::null();
    }
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
        return static_cast<int>(std::get<Util::SimulationClock::time_point>(*this).time_since_epoch().count());
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
        Util::SimulationClock::time_format = Util::SimulationClock::Format::NO_CLOCK_AMERICAN;
        auto time_point = std::get<Util::SimulationClock::time_point>(*this);
        std::ostringstream stream;
        stream << time_point;
        return stream.str();
    }
    }
    __builtin_unreachable();
}

DbErrorOr<bool> Value::to_bool() const {
    return TRY(to_int()) != 0;
}

DbErrorOr<Util::SimulationClock::time_point> Value::to_time() const {
    if (type() != Type::Time) {
        return DbError { "Cannot convert value to time", 0 };
    }
    return std::get<Util::SimulationClock::time_point>(*this);
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
    }
    __builtin_unreachable();
}

void Value::repl_dump(std::ostream& out) const {
    out << to_debug_string() << std::endl;
}

std::ostream& operator<<(std::ostream& out, Value const& value) {
    auto error = value.to_string();
    if (error.is_error())
        return out << "<invalid>";
    return out << error.release_value();
}

DbErrorOr<Value> operator+(Value const& lhs, Value const& rhs) {
    if (lhs.is_null() || rhs.is_null())
        return Value::null();
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
        Util::SimulationClock::duration dur(time);
        return Value::create_time(Util::SimulationClock::time_point(dur));
    }
    }
    __builtin_unreachable();
}

DbErrorOr<Value> operator-(Value const& lhs, Value const& rhs) {
    if (lhs.is_null() || rhs.is_null())
        return Value::null();
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
        Util::SimulationClock::duration dur(time);
        return Value::create_time(Util::SimulationClock::time_point(dur));
    }
    }
    __builtin_unreachable();
}

DbErrorOr<Value> operator*(Value const& lhs, Value const& rhs) {
    if (lhs.is_null() || rhs.is_null())
        return Value::null();
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
    }
    __builtin_unreachable();
}

DbErrorOr<Value> operator/(Value const& lhs, Value const& rhs) {
    if (lhs.is_null() || rhs.is_null())
        return Value::null();
    switch (lhs.type()) {
    case Value::Type::Bool:
        return Value::create_bool(TRY(lhs.to_bool()) / TRY(rhs.to_bool()));
    case Value::Type::Int: {
        auto rhs_int = TRY(rhs.to_int());
        if (rhs_int == 0)
            return DbError { "Cannot divide by 0", 0 };
        return Value::create_int(TRY(lhs.to_int()) / rhs_int);
    }
    case Value::Type::Float: {
        auto rhs_float = TRY(rhs.to_float());
        if (rhs_float == 0)
            return DbError { "Cannot divide by 0", 0 };
        return Value::create_float(TRY(lhs.to_float()) / rhs_float);
    }
    case Value::Type::Null:
        return Value::null();
    case Value::Type::Varchar:
        return DbError { "No matching operator '/' for 'VARCHAR' type.", 0 };
    case Value::Type::Time:
        return DbError { "No matching operator '/' for 'TIME' type.", 0 };
    }
    __builtin_unreachable();
}

DbErrorOr<bool> operator<(Value const& lhs, Value const& rhs) {
    if (lhs.is_null())
        return true;
    if (rhs.is_null())
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
        return rhs.is_null();
    case Value::Type::Varchar:
        return TRY(lhs.to_string()) == TRY(rhs.to_string());
    case Value::Type::Time:
        return TRY(lhs.to_int()) == TRY(rhs.to_int());
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
