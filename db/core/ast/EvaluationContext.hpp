#pragma once

#include <db/core/ast/SelectColumns.hpp>

namespace Db::Core {
class Database;
}

namespace Db::Core::AST {

class TableExpression;

struct EvaluationContextFrame {
    TableExpression const* table = nullptr;
    SelectColumns const& columns;
    TupleWithSource row {};

    std::optional<std::span<Tuple const>> row_group {};
    enum class RowType {
        FromTable,
        FromResultSet
    };
    RowType row_type = RowType::FromTable;

    EvaluationContextFrame(TableExpression const* table_, SelectColumns const& columns_)
        : table(table_)
        , columns(columns_) { }
};

struct EvaluationContext {
    Database* db = nullptr;
    std::list<EvaluationContextFrame> frames {};

    EvaluationContextFrame& current_frame() {
        assert(!frames.empty());
        return frames.back();
    }
};

}
