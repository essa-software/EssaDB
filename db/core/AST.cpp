#include "AST.hpp"

#include "Database.hpp"

#include <iostream>

namespace Db::Core::AST {

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context, Row const& row) const {
    auto column = context.table.get_column(m_id);
    if (!column)
        return DbError { "No such column: " + m_id };
    return row.value(column->second);
}

DbErrorOr<Value> Select::execute(Database& db) const {
    // Comments specify SQL Conceptional Evaluation:
    // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql#logical-processing-order-of-the-select-statement
    // FROM
    auto table = TRY(db.table(m_from));

    auto should_include_row = [&](Row const& row) -> DbErrorOr<bool> {
        if (!m_where)
            return true;
        return TRY(m_where->is_true(row.value(table->get_column(m_where->column)->second)));
    };

    // TODO: ON
    // TODO: JOIN

    EvaluationContext context { .table = *table };

    std::vector<Row> rows;
    for (auto const& row : table->rows()) {
        // WHERE
        if (!TRY(should_include_row(row)))
            continue;

        // TODO: GROUP BY
        // TODO: HAVING

        // SELECT
        std::vector<Value> values;
        for (auto const& column : m_columns.columns()) {
            values.push_back(TRY(column->evaluate(context, row)));
        }
        rows.push_back(Row { values });
    }

    // TODO: DISTINCT

    // ORDER BY
    if (m_order_by) {
        auto order_by_column = table->get_column(m_order_by->column_name)->second;
        if (!order_by_column)
            return DbError { "Invalid column to order by: " + m_order_by->column_name };
        std::stable_sort(rows.begin(), rows.end(), [&](Row const& lhs, Row const& rhs) -> bool {
            // TODO: Do sorting properly
            auto lhs_value = lhs.value(order_by_column).to_string();
            auto rhs_value = rhs.value(order_by_column).to_string();
            if (lhs_value.is_error() || rhs_value.is_error()) {
                // TODO: Actually handle error
                return false;
            }
            return (lhs_value.release_value() > rhs_value.release_value()) == (m_order_by->order == OrderBy::Order::Ascending);
        });
    }

    if (m_top) {
        if (m_top->unit == Top::Unit::Perc) {
            float mul = static_cast<float>(std::min(m_top->value, (unsigned)100)) / 100;
            rows.resize(rows.size() * mul, rows.back());
        }
        else {
            rows.resize(std::min(m_top->value, (unsigned)rows.size()), rows.back());
        }
    }

    std::vector<std::string> column_names;
    if (m_columns.select_all()) {
        for (auto const& column : table->columns())
            column_names.push_back(column.name());
    }
    else {
        for (auto const& column : m_columns.columns())
            column_names.push_back(column->to_string());
    }

    return Value::create_select_result({ column_names, std::move(rows) });
}

}
