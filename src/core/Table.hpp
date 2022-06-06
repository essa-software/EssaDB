#pragma once

#include "Column.hpp"
#include "DbError.hpp"
#include "SelectResult.hpp"

#include <set>
#include <util/NonCopyable.hpp>

namespace Db::Core {

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

struct Query {
    std::set<std::string> columns {};
    bool select_all = false;
    std::optional<Filter> filter = {};
};

class Table : public Util::NonCopyable {
public:
    std::optional<std::pair<Column, size_t>> get_column(std::string const& name) const;
    size_t size() const { return m_rows.size(); }
    std::vector<Column> const& columns() const { return m_columns; }

    DbErrorOr<void> add_column(Column);
    DbErrorOr<void> insert(RowWithColumnNames::MapType);

    DbErrorOr<SelectResult> select(Query = { .select_all = true }) const;

private:
    std::vector<Row> m_rows;
    std::vector<Column> m_columns;
};

}
