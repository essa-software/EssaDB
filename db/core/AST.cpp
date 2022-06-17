#include "AST.hpp"

#include "Database.hpp"
#include "DbError.hpp"
#include "Function.hpp"
#include "Tuple.hpp"
#include "Value.hpp"
#include "db/core/RowWithColumnNames.hpp"

#include <db/util/Is.hpp>
#include <iostream>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Db::Core::AST {

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context, Tuple const& row) const {
    auto column = context.table.get_column(m_id);
    if (!column)
        return DbError { "No such column: " + m_id, start() };
    return row.value(column->second);
}

DbErrorOr<bool> BinaryOperator::is_true(EvaluationContext& context, Tuple const& row) const {
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
    case Operation::Not:
        return !TRY(TRY(m_lhs->evaluate(context, row)).to_bool());
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
    case Operation::Invalid:
        break;
    }
    __builtin_unreachable();
}

DbErrorOr<Value> BetweenExpression::evaluate(EvaluationContext& context, Tuple const& row) const {
    // TODO: Implement this for strings etc
    auto value = TRY(TRY(m_lhs->evaluate(context, row)).to_int());
    auto min = TRY(TRY(m_min->evaluate(context, row)).to_int());
    auto max = TRY(TRY(m_max->evaluate(context, row)).to_int());
    return Value::create_bool(value >= min && value <= max);
}

DbErrorOr<Value> InExpression::evaluate(EvaluationContext& context, Tuple const& row) const {
    // TODO: Implement this for strings etc
    auto value = TRY(TRY(m_lhs->evaluate(context, row)).to_string());
    for(const auto& arg : m_args){
        auto to_compare = TRY(TRY(arg->evaluate(context, row)).to_string());

        if(value == to_compare)
            return Value::create_bool(true);;
    }
    return Value::create_bool(false);
}

DbErrorOr<Value> Select::execute(Database& db) const {
    // Comments specify SQL Conceptional Evaluation:
    // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql#logical-processing-order-of-the-select-statement
    // FROM
    auto table = TRY(db.table(m_from));

    EvaluationContext context { .table = *table };

    bool is_aggregate = m_group_by.has_value();

    for (auto& it : m_columns.columns()) {
        if (Util::is<AggregateFunction>(*it.column)) {
            is_aggregate |= true;
        }else if(m_group_by){
            is_aggregate |= true;
        }
        else {
            if(is_aggregate)
                return DbError { "All columns must be either aggregate or non-aggregate", start() };
        }
    }
    std::vector<Tuple> rows;

    if(!is_aggregate){
        auto should_include_row = [&](Tuple const& row) -> DbErrorOr<bool> {
            if (!m_where)
                return true;
            return TRY(m_where->evaluate(context, row)).to_bool();
        };

        // TODO: ON
        // TODO: JOIN

        for (auto const& row : table->rows()) {
            // WHERE
            if (!TRY(should_include_row(row)))
                continue;

            // SELECT
            std::vector<Value> values;
            if (m_columns.select_all()) {
                for (auto const& column : table->columns()) {
                    auto table_column = table->get_column(column.name());
                    if (!table_column)
                        return DbError { "Internal error: invalid column requested for *: '" + column.name() + "'", start() + 1 };
                    values.push_back(row.value(table_column->second));
                }
            }
            else {
                for (auto const& column : m_columns.columns()) {
                    values.push_back(TRY(column.column->evaluate(context, row)));
                }
            }
            rows.push_back(Tuple { values });
        }
    }
    else { // Aggregation + GROUP BY
        std::map<Tuple, std::vector<Tuple>> groups;
        Table aggregate_table;

        for(const auto& row : table->rows()){
            std::vector<Value> values;

            if(m_group_by){
                for(const auto& column_name : m_group_by->columns){
                    auto column = table->get_column(column_name);
                    if (!column)
                        return DbError { "Internal error: invalid column requested for *: '" + column_name + "'", start() + 1 };
                    values.push_back(row.value(column->second));
                }
            }

            Tuple key(std::span(values.data(), values.size()));
            auto it = groups.begin();

            for(it = groups.begin(); it != groups.end(); it++){
                if(it->first == key){
                    it->second.push_back(row);
                    break;
                }
            }

            if(it == groups.end()){
                groups.insert({key, std::vector<Tuple>(1, row)});
            }
        }

        if(m_group_by){
            for(const auto& column_name : m_group_by->columns){
                auto column = table->get_column(column_name);
                if (!column)
                    return DbError { "Internal error: invalid column requested for *: '" + column_name + "'", start() + 1 };
                TRY(aggregate_table.add_column(column->first));
            }

            RowWithColumnNames::MapType map;
            for(const auto& group : groups){
                for(size_t i = 0; i < m_group_by->columns.size(); i++){
                    map.insert({m_group_by->columns[i], group.first.value(i)});
                }
                TRY(aggregate_table.insert(std::move(map)));
            }
        }

        EvaluationContext aggregate_context { .table = aggregate_table };

        for(const auto& group : groups){
            std::vector<Value> aggregate_values;

            for (auto& it : m_columns.columns()) {
                if (Util::is<AggregateFunction>(*it.column)) {
                    // std::cout << group.first.control_number << "\n";
                    aggregate_values.push_back(TRY(static_cast<AggregateFunction&>(*it.column).aggregate(context, group.second)));
                }else if(m_group_by){
                    aggregate_values.push_back(TRY(it.column->evaluate(aggregate_context, group.first)));
                }
            }

            rows.push_back( Tuple { aggregate_values } );
        }
    }

    // DISTINCT
    if (m_distinct) {
        std::vector<Tuple> occurences;

        for (const auto& row : rows) {
            bool distinct = true;
            for (const auto& to_compare : occurences) {
                if (row == to_compare) {
                    distinct = false;
                    break;
                }
            }

            if (distinct)
                occurences.push_back(row);
        }

        rows = std::move(occurences);
    }

    // ORDER BY
    if (m_order_by) {
        for (const auto& column : m_order_by->columns) {
            auto order_by_column = table->get_column(column.name)->second;
            if (!order_by_column) {
                // TODO: Store source position info in ORDER BY node
                return DbError { "Invalid column to order by: " + column.name, start() };
            }
        }
        std::stable_sort(rows.begin(), rows.end(), [&](Tuple const& lhs, Tuple const& rhs) -> bool {
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

DbErrorOr<Value> DeleteFrom::execute(Database& db) const {
    auto table = TRY(db.table(m_from));

    EvaluationContext context { .table = *table };

    auto should_include_row = [&](Tuple const& row) -> DbErrorOr<bool> {
        if (!m_where)
            return true;
        return TRY(m_where->evaluate(context, row)).to_bool();
    };
label:;

    for (size_t i = 0; i < table->rows().size(); i++) {
        if (TRY(should_include_row(table->rows()[i]))) {
            table->delete_row(i);

            goto label;
        }
    }

    return Value::null();
}

DbErrorOr<Value> CreateTable::execute(Database& db) const {
    auto& table = db.create_table(m_name);
    for (auto const& column : m_columns) {
        TRY(table.add_column(column));
    }
    return { Value::null() };
}

DbErrorOr<Value> DropTable::execute(Database& db) const {
    TRY(db.drop_table(m_name));

    return { Value::null() };
}

DbErrorOr<Value> TruncateTable::execute(Database& db) const {
    auto table = TRY(db.table(m_name));
    table->truncate();

    return { Value::null() };
}

DbErrorOr<Value> AlterTable::execute(Database& db) const {
    auto table = TRY(db.table(m_name));

    for (const auto& to_add : m_to_add) {
        TRY(table->add_column(to_add));
    }

    for (const auto& to_alter : m_to_alter) {
        TRY(table->alter_column(to_alter));
    }

    for (const auto& to_drop : m_to_drop) {
        TRY(table->drop_column(to_drop));
    }

    return { Value::null() };
}

DbErrorOr<Value> InsertInto::execute(Database& db) const {
    auto table = TRY(db.table(m_name));

    RowWithColumnNames::MapType map;
    EvaluationContext context { .table = *table };
    if (m_select) {
        auto result = TRY(TRY(m_select->value()->execute(db)).to_select_result());

        if (m_columns.size() != result.column_names().size())
            return DbError { "Values doesn't have corresponding columns", start() };

        for (const auto& row : result.rows()) {
            for (size_t i = 0; i < m_columns.size(); i++) {
                map.insert({ m_columns[i], row.value(i) });
            }

            TRY(table->insert(std::move(map)));
        }
    }
    else {
        if (m_columns.size() != m_values.size())
            return DbError { "Values doesn't have corresponding columns", start() };

        for (size_t i = 0; i < m_columns.size(); i++) {
            map.insert({ m_columns[i], TRY(m_values[i]->evaluate(context, Tuple({}))) });
        }

        TRY(table->insert(std::move(map)));
    }
    return { Value::null() };
}

}
