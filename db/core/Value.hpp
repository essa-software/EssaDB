#pragma once

#include "DbError.hpp"
#include "ResultSet.hpp"
#include <EssaUtil/SimulationClock.hpp>

#include <optional>
#include <string>
#include <variant>

namespace Db::Core {

class ResultSet;

using ValueBase = std::variant<std::monostate, int, float, std::string, bool, Util::SimulationClock::time_point, ResultSet>;

class Value : public ValueBase {
public:
    enum class Type {
        Null,
        Int,
        Float,
        Varchar,
        Bool,
        Time,
        SelectResult,
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

    Value() = default;
    static DbErrorOr<Value> from_string(Type, std::string const&);
    static Value null();
    static Value create_int(int i);
    static Value create_float(float f);
    static Value create_varchar(std::string s);
    static Value create_bool(bool b);
    static Value create_time(Util::SimulationClock::time_point clock);
    static Value create_time(std::string str, Util::SimulationClock::Format format);
    static Value create_select_result(ResultSet);

    DbErrorOr<int> to_int() const;
    DbErrorOr<float> to_float() const;
    DbErrorOr<std::string> to_string() const;
    DbErrorOr<bool> to_bool() const;
    DbErrorOr<ResultSet> to_select_result() const;

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

}
