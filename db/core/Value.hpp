#pragma once

#include "DbError.hpp"
#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace Db::Core {

class ResultSet;

struct Date {
    int year;
    int month;
    int day;

    time_t to_utc_epoch() const;
    static Date from_utc_epoch(time_t d);
    static DbErrorOr<Date> from_iso8601_string(std::string const& string);
};

using ValueBase = std::variant<std::monostate, int, float, std::string, bool, Date>;

class Value : public ValueBase {
public:
    enum class Type {
        Null,
        Int,
        Float,
        Varchar,
        Bool,
        Time,
    };

    static std::optional<Type> type_from_string(std::string const& str) {
        if (str == "INT")
            return Type::Int;
        if (str == "FLOAT")
            return Type::Float;
        if (str == "VARCHAR")
            return Type::Varchar;
        if (str == "BOOL")
            return Type::Bool;
        if (str == "TIME")
            return Type::Time;
        return {};
    }

    static std::string type_to_string(Type type) {
        switch (type) {
        case Type::Int:
            return "INT";
        case Type::Float:
            return "FLOAT";
        case Type::Varchar:
            return "VARCHAR";
        case Type::Bool:
            return "BOOL";
        case Type::Time:
            return "TIME";
        case Type::Null:
            return "NULL";
        }
        ESSA_UNREACHABLE;
    }

    Value() = default;
    static DbErrorOr<Value> from_string(Type, std::string const&);
    static Value null();
    static Value create_int(int i);
    static Value create_float(float f);
    static Value create_varchar(std::string s);
    static Value create_bool(bool b);
    static Value create_time(Date);

    DbErrorOr<int> to_int() const;
    DbErrorOr<float> to_float() const;
    DbErrorOr<std::string> to_string() const;
    DbErrorOr<bool> to_bool() const;
    DbErrorOr<Date> to_time() const;

    Type type() const { return m_type; }
    bool is_null() const { return m_type == Value::Type::Null; }

    std::string to_debug_string() const;

    void repl_dump(std::ostream& out) const;
    friend std::ostream& operator<<(std::ostream& out, Value const&);

private:
    Value(auto value, Type type)
        : ValueBase(std::move(value))
        , m_type(type) { }

    Type m_type { Type::Null };
};

DbErrorOr<Value> operator+(Value const& lhs, Value const& rhs);
DbErrorOr<Value> operator-(Value const& lhs, Value const& rhs);
DbErrorOr<Value> operator*(Value const& lhs, Value const& rhs);
DbErrorOr<Value> operator/(Value const& lhs, Value const& rhs);

DbErrorOr<bool> operator<(Value const& lhs, Value const& rhs);
DbErrorOr<bool> operator<=(Value const& lhs, Value const& rhs);
DbErrorOr<bool> operator==(Value const& lhs, Value const& rhs);
DbErrorOr<bool> operator>=(Value const& lhs, Value const& rhs);
DbErrorOr<bool> operator>(Value const& lhs, Value const& rhs);
DbErrorOr<bool> operator!=(Value const& lhs, Value const& rhs);

Value::Type find_type(const std::string& str);

class ValueSorter : public std::less<Value> {
public:
    bool operator()(const Value& lhs, const Value& rhs) const {
        auto result = lhs < rhs;
        if (result.is_error())
            return false;
        return result.release_value();
    }
};

}
