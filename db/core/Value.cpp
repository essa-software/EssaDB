#include "Value.hpp"

#include "DbError.hpp"
#include "Regex.hpp"
#include "ResultSet.hpp"
#include "Tuple.hpp"

#include <EssaUtil/Config.hpp>
#include <db/sql/Printing.hpp>

#include <cctype>
#include <ctime>
#include <iostream>
#include <limits>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>

namespace Db::Core {

tm Date::to_tm_struct() const{
    tm tm {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_zone = "UTC";

    return tm;
}

time_t Date::to_utc_epoch() const {
    tm tm = to_tm_struct();
    return mktime(&tm);
}

Date Date::from_utc_epoch(time_t time) {
    auto tm = gmtime(&time);

    return {
        .year = tm->tm_year + 1900,
        .month = tm->tm_mon + 1,
        .day = tm->tm_mday,
        .hour = tm->tm_hour,
        .min = tm->tm_min,
        .sec = tm->tm_sec
    };
}

Date Date::from_local_epoch(time_t time) {
    auto tm = localtime(&time);

    return {
        .year = tm->tm_year + 1900,
        .month = tm->tm_mon + 1,
        .day = tm->tm_mday,
        .hour = tm->tm_hour,
        .min = tm->tm_min,
        .sec = tm->tm_sec
    };
}

Value::Type find_type(const std::string& str) {
    if (str == "null")
        return Value::Type::Null;

    for (const auto& c : str) {
        if (c < '0' || c > '9')
            return Value::Type::Varchar;
    }

    return Value::Type::Int;
}

DbErrorOr<Date> Date::from_iso8601_string(std::string const& string) {
    std::regex regex { "^(\\d{4})-(\\d{2})-(\\d{2}) (\\d{1,2}):(\\d{2}):(\\d{2})$" };
    std::smatch match;
    std::string str = string + (string.length() == 10 ? " 00:00:00" : "");
    if (!std::regex_search(str, match, regex)) {
        return DbError { "Invalid ISO8601 date" };
    }
    Date date;
    date.year = std::stoi(match[1]);
    date.month = std::stoi(match[2]);
    if (date.month < 1 || date.month > 12) {
        return DbError { fmt::format("Month must be in range 1..12, {} given", date.month) };
    }
    date.day = std::stoi(match[3]);
    if (date.day < 1 || date.day > 31) {
        return DbError { fmt::format("Date must be in range 1..31, {} given", date.day) };
    }
    date.hour = std::stoi(match[4]);
    if (date.hour < 0 || date.hour > 23) {
        return DbError { fmt::format("Hours must be in range 0..23, {} given", date.hour) };
    }
    date.min = std::stoi(match[5]);
    if (date.min < 0 || date.min > 59) {
        return DbError { fmt::format("Minutes must be in range 0..59, {} given", date.min) };
    }
    date.sec = std::stoi(match[6]);
    if (date.min < 0 || date.sec > 59) {
        return DbError { fmt::format("Seconds must be in range 0..59, {} given", date.sec) };
    }
    return date;
}

DbErrorOr<Value> Value::from_string(Type type, std::string const& string) {
    switch (type) {
    case Type::Null:
        return DbError { "Internal error: trying to parse string into null type" };
    case Type::Int:
        try {
            return Value::create_int(std::stoi(string));
        } catch (...) {
            return DbError { "Cannot convert '" + string + "' to int" };
        }
    case Type::Float:
        try {
            return Value::create_float(std::stof(string));
        } catch (...) {
            return DbError { "Cannot convert '" + string + "' to float" };
        }
    case Type::Varchar:
        return Value::create_varchar(string);
    case Type::Bool:
        if (string == "true")
            return Value::create_bool(true);
        else if (string == "false")
            return Value::create_bool(false);
        return DbError { "Bool value must be either 'true' or 'false', got '" + string + "'" };
    case Type::Time:
        return Value::create_time(TRY(Date::from_iso8601_string(string)));
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

Value Value::create_time(Date t) {
    return Value { t, Type::Time };
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
            return DbError { "'" + str + "' is not a valid int" };
        }
    }
    case Type::Bool:
        return std::get<bool>(*this) ? 1 : 0;
    case Type::Time: {
        auto range = std::get<Date>(*this).to_utc_epoch();
        if (range > std::numeric_limits<int>::max()) {
            // Fix your database until it hits y2k38...
            return DbError { "Timestamp out of range for int type" };
        }
        return static_cast<int>(range);
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
            return DbError { "'" + str + "' is not a valid float" };
        }
    }
    case Type::Bool:
        return std::get<bool>(*this) ? 1.f : 0.f;
    case Type::Time:
        return DbError { "Time is not convertible to float" };
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
        auto time_point = std::get<Date>(*this);
        return fmt::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", time_point.year, time_point.month, time_point.day, time_point.hour, time_point.min, time_point.sec);
    }
    }
    __builtin_unreachable();
}

DbErrorOr<bool> Value::to_bool() const {
    return TRY(to_int()) != 0;
}

DbErrorOr<Date> Value::to_time() const {
    if (type() != Type::Time) {
        return DbError { "Cannot convert value to time" };
    }
    return std::get<Date>(*this);
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

std::string Value::to_sql_serialized_string() const {
    switch (m_type) {
    case Type::Null:
        return "NULL";
    case Type::Int:
    case Type::Float:
    case Type::Bool:
    case Type::Time:
        return MUST(to_string());
    case Type::Varchar:
        return Sql::Printing::escape_string_literal(std::get<std::string>(*this));
    }
    ESSA_UNREACHABLE;
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
        time_t time = std::get<Date>(lhs).to_utc_epoch();
        time += TRY(rhs.to_int());
        return Value::create_time(Date::from_utc_epoch(time));
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
        return DbError { "No matching operator '-' for 'VARCHAR' type." };
    case Value::Type::Time: {
        time_t time = std::get<Date>(lhs).to_utc_epoch();
        time -= TRY(rhs.to_int());
        return Value::create_time(Date::from_utc_epoch(time));
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
        return DbError { "No matching operator '*' for 'VARCHAR' type." };
    case Value::Type::Time:
        return DbError { "No matching operator '*' for 'TIME' type." };
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
            return DbError { "Cannot divide by 0" };
        return Value::create_int(TRY(lhs.to_int()) / rhs_int);
    }
    case Value::Type::Float: {
        auto rhs_float = TRY(rhs.to_float());
        if (rhs_float == 0)
            return DbError { "Cannot divide by 0" };
        return Value::create_float(TRY(lhs.to_float()) / rhs_float);
    }
    case Value::Type::Null:
        return Value::null();
    case Value::Type::Varchar:
        return DbError { "No matching operator '/' for 'VARCHAR' type." };
    case Value::Type::Time:
        return DbError { "No matching operator '/' for 'TIME' type." };
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
