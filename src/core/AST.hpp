#pragma once

#include "SelectResult.hpp"
#include "Value.hpp"

#include <set>

namespace Db::Core {
class Database;
}

namespace Db::Core::AST {

struct Filter {
    std::string column;
    enum class Operation {
        Equal,        // =
        NotEqual,     // !=
        Greater,      // >
        GreaterEqual, // >=
        Less,         // <
        LessEqual,    // <=
    };
    Operation operation;
    Value rhs;

    DbErrorOr<bool> is_true(Value const& lhs) const {
        switch (operation) {
        case Operation::Equal:
            return TRY(lhs.to_string()) == TRY(rhs.to_string());
        case Operation::NotEqual:
            return TRY(lhs.to_string()) != TRY(rhs.to_string());
        case Operation::Greater:
            return TRY(lhs.to_string()) > TRY(rhs.to_string());
        case Operation::GreaterEqual:
            return TRY(lhs.to_string()) >= TRY(rhs.to_string());
        case Operation::Less:
            return TRY(lhs.to_string()) < TRY(rhs.to_string());
        case Operation::LessEqual:
            return TRY(lhs.to_string()) <= TRY(rhs.to_string());
        }
        __builtin_unreachable();
    }
};

class SelectColumns {
public:
    SelectColumns() = default;
    SelectColumns(std::set<std::string> columns)
        : m_columns(std::move(columns)) { }

    bool select_all() const { return m_columns.empty(); }
    bool has(std::string const& column_name) const { return select_all() || m_columns.contains(column_name); }

private:
    std::set<std::string> m_columns {};
};

class Select {
public:
    Select(SelectColumns columns, std::string from, std::optional<Filter> where)
        : m_columns(std::move(columns))
        , m_from(std::move(from))
        , m_where(std::move(where)) { }

    DbErrorOr<SelectResult> execute(Database&) const;

private:
    SelectColumns m_columns;
    std::string m_from;
    std::optional<Filter> m_where;
};

}
