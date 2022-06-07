#pragma once

#include "DbError.hpp"
#include "SelectResult.hpp"

#include <optional>
#include <string>
#include <variant>

namespace Db::Core {

class SelectResult;

using ValueBase = std::variant<std::monostate, int, std::string, SelectResult>;

class Value : public ValueBase {
public:
    enum class Type {
        Null,
        Int,
        Varchar,
        SelectResult,
    };

    Value() = default;
    static Value null();
    static Value create_int(int i);
    static Value create_varchar(std::string s);
    static Value create_select_result(SelectResult);

    DbErrorOr<int> to_int() const;
    DbErrorOr<std::string> to_string() const;
    std::string to_debug_string() const;

    Type type() const { return m_type; }

    void repl_dump(std::ostream& out) const;
    friend std::ostream& operator<<(std::ostream& out, Value const&);

private:
    Value(auto value, Type type)
        : ValueBase(std::move(value))
        , m_type(type) { }

    Type m_type { Type::Null };
};

}
