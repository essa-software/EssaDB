#include "AST.hpp"

#include "Database.hpp"
#include "db/core/Value.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace Db::Core::AST {

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context, Row const& row) const {
    auto column = context.table.get_column(m_id);
    if (!column)
        return DbError { "No such column: " + m_id };
    return row.value(column->second);
}

DbErrorOr<bool> BinaryOperator::is_true(EvaluationContext& context, Row const& row) const {
    // TODO: Implement proper comparison
    switch (m_operation) {
    case Operation::Equal:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_string()) == TRY(TRY(m_rhs->evaluate(context, row)).to_string());
    case Operation::NotEqual:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_string()) != TRY(TRY(m_rhs->evaluate(context, row)).to_string());
    case Operation::Greater:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_string()) > TRY(TRY(m_rhs->evaluate(context, row)).to_string());
    case Operation::GreaterEqual:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_string()) >= TRY(TRY(m_rhs->evaluate(context, row)).to_string());
    case Operation::Less:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_string()) < TRY(TRY(m_rhs->evaluate(context, row)).to_string());
    case Operation::LessEqual:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_string()) <= TRY(TRY(m_rhs->evaluate(context, row)).to_string());
    case Operation::And:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_bool()) && TRY(TRY(m_rhs->evaluate(context, row)).to_bool());
    case Operation::Or:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_bool()) || TRY(TRY(m_rhs->evaluate(context, row)).to_bool());
    case Operation::Like: {
        std::string str = TRY(TRY(m_lhs->evaluate(context, row)).to_string());
        std::string to_compare = TRY(TRY(m_rhs->evaluate(context, row)).to_string());

        if (to_compare.front() == '*' && to_compare.back() == '*') {
            std::string comparison_substr = to_compare.substr(1, to_compare.size() - 2);

            if (str.size() - 1 < to_compare.size())
                return false;

            return str.find(comparison_substr) != std::string::npos;
        }
        else if (to_compare.front() == '*') {
            auto it1 = str.end(), it2 = to_compare.end();

            if (str.size() < to_compare.size())
                return false;

            while (it1 != str.begin()) {
                if (*it2 == '*')
                    break;

                if (*it1 != *it2 && *it2 != '?')
                    return false;
                it1--;
                it2--;
            }
        }
        else if (to_compare.back() == '*') {
            auto it1 = str.begin(), it2 = to_compare.begin();

            if (str.size() < to_compare.size())
                return false;

            while (it1 != str.end()) {
                if (*it2 == '*')
                    break;

                if (*it1 != *it2 && *it2 != '?')
                    return false;
                it1++;
                it2++;
            }
        }
        else {
            auto it1 = str.begin(), it2 = to_compare.begin();
            if (str.size() != to_compare.size())
                return false;

            while (it1 != str.end()) {
                if (*it1 != *it2 && *it2 != '?')
                    return false;
                it1++;
                it2++;
            }
        }

        return true;
    }
    }
    __builtin_unreachable();
}

DbErrorOr<Value> Select::execute(Database& db) const {
    // Comments specify SQL Conceptional Evaluation:
    // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql#logical-processing-order-of-the-select-statement
    // FROM
    auto table = TRY(db.table(m_from));

    EvaluationContext context { .table = *table };

    auto should_include_row = [&](Row const& row) -> DbErrorOr<bool> {
        if (!m_where)
            return true;
        return TRY(m_where->evaluate(context, row)).to_bool();
    };

    // TODO: ON
    // TODO: JOIN

    std::vector<Row> rows;
    for (auto const& row : table->rows()) {
        // WHERE
        if (!TRY(should_include_row(row)))
            continue;

        // TODO: GROUP BY
        // TODO: HAVING

        // SELECT
        std::vector<Value> values;
        if (m_columns.select_all()) {
            for (auto const& column : table->columns()) {
                auto table_column = table->get_column(column.name());
                if (!table_column)
                    return DbError { "Internal error: invalid column requested for *: '" + column.name() + "'" };
                values.push_back(row.value(table_column->second));
            }
        }
        else {
            for (auto const& column : m_columns.columns()) {
                values.push_back(TRY(column.column->evaluate(context, row)));
            }
        }
        rows.push_back(Row { values });
    }

    // TODO: DISTINCT

    // ORDER BY
    if (m_order_by) {
        for (const auto& column : m_order_by->columns) {
            auto order_by_column = table->get_column(column.name)->second;
            if (!order_by_column)
                return DbError { "Invalid column to order by: " + column.name };
        }
        std::stable_sort(rows.begin(), rows.end(), [&](Row const& lhs, Row const& rhs) -> bool {
            // TODO: Do sorting properly
            for (const auto& column : m_order_by->columns) {
                auto order_by_column = table->get_column(column.name)->second;

                auto lhs_value = lhs.value(order_by_column).to_string();
                auto rhs_value = rhs.value(order_by_column).to_string();
                if (lhs_value.is_error() || rhs_value.is_error()) {
                    // TODO: Actually handle error
                    return false;
                }

                if (lhs_value.value() != rhs_value.value())
                    return (lhs_value.release_value() < rhs_value.release_value()) == (column.order == OrderBy::Order::Ascending);
            }

            return false;
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
        for (auto const& column : m_columns.columns()) {
            if (column.alias)
                column_names.push_back(*column.alias);
            else
                column_names.push_back(column.column->to_string());
        }
    }

    return Value::create_select_result({ column_names, std::move(rows) });
}

DbErrorOr<Value> CreateTable::execute(Database& db) const {
    auto& table = db.create_table(m_name);
    for (auto const& column : m_columns) {
        TRY(table.add_column(column));
    }
    return { Value::null() };
}

}
