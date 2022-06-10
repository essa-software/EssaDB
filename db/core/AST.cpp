#include "AST.hpp"

#include "Database.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace Db::Core::AST {


DbErrorOr<bool> Filter::is_true(FilterSet const& rule, Value const& lhs) const {
switch (rule.operation) {
    case Operation::Equal:
        return TRY(lhs.to_string()) == TRY(rule.args[0].value().to_string());
    case Operation::NotEqual:
        return TRY(lhs.to_string()) != TRY(rule.args[0].value().to_string());
    case Operation::Greater:
        return TRY(lhs.to_string()) > TRY(rule.args[0].value().to_string());
    case Operation::GreaterEqual:
        return TRY(lhs.to_string()) >= TRY(rule.args[0].value().to_string());
    case Operation::Less:
        return TRY(lhs.to_string()) < TRY(rule.args[0].value().to_string());
    case Operation::LessEqual:
        return TRY(lhs.to_string()) <= TRY(rule.args[0].value().to_string());
    case Operation::Like:{
        std::string str = lhs.to_string().value();
        std::string to_compare = rule.args[0].value().to_string().value();

        if(to_compare.front() == '*' && to_compare.back() == '*'){
            std::string comparison_substr = to_compare.substr(1, to_compare.size() - 2);

            if(str.size() - 1 < to_compare.size())
                return false;
            
            return str.find(comparison_substr) != std::string::npos;
        }else if(to_compare.front() == '*'){
            auto it1 = str.end(), it2 = to_compare.end();

            if(str.size() < to_compare.size())
                return false;

            while (it1 != str.begin()) {
                if(*it2 == '*')
                    break;

                if(*it1 != *it2 && *it2 != '?')
                    return false;
                it1--;
                it2--;
            }
        }else if(to_compare.back() == '*'){
            auto it1 = str.begin(), it2 = to_compare.begin();

            if(str.size() < to_compare.size())
                return false;

            while (it1 != str.end()) {
                if(*it2 == '*')
                    break;

                if(*it1 != *it2 && *it2 != '?')
                    return false;
                it1++;
                it2++;
            }
        }else{
            auto it1 = str.begin(), it2 = to_compare.begin();
            if(str.size() != to_compare.size())
                return false;

            while (it1 != str.end()) {
                if(*it1 != *it2 && *it2 != '?')
                    return false;
                it1++;
                it2++;
            }
        }

        return true;
    }case Operation::Between:
        return TRY(lhs.to_string()) >= TRY(rule.args[0].value().to_string()) && TRY(lhs.to_string()) <= TRY(rule.args[1].value().to_string());
    case Operation::In:
        for(const auto& arg : rule.args){
            if(TRY(lhs.to_string()) == TRY(arg.value().to_string()))
                return true;
        }
        return false;
    }
    __builtin_unreachable();
}

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context, Row const& row) const {
    auto column = context.table.get_column(m_id);
    if (!column)
        return DbError { "No such column: " + m_id };
    return row.value(column->second);
}

DbErrorOr<Value> Function::evaluate(EvaluationContext& context, Row const& row) const {
    if (m_name == "LEN") {
        // https://www.w3schools.com/sqL/func_sqlserver_len.asp
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: string" };
        auto arg = TRY(m_args[0]->evaluate(context, row));
        switch (arg.type()) {
        case Value::Type::Null:
            // If string is NULL, it returns NULL
            return Value::null();
        default:
            // FIXME: What to do with ints?
            return Value::create_int(TRY(arg.to_string()).size());
        }
    }

    return DbError { "Undefined function: '" + m_name + "'" };
}

DbErrorOr<Value> Select::execute(Database& db) const {
    // Comments specify SQL Conceptional Evaluation:
    // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql#logical-processing-order-of-the-select-statement
    // FROM
    auto table = TRY(db.table(m_from));

    auto should_include_row = [&](Row const& row) -> DbErrorOr<bool> {
        if (!m_where)
            return true;

        bool result = 0, condition = 1;
        unsigned counter = 0;
        std::vector<bool> values;

        for(const auto& rule : m_where->filter_rules){
            if(rule.logic == Filter::LogicOperator::AND){
                condition *= m_where->is_true(rule, row.value(table->get_column(rule.column)->second)).value();
                counter++;
            }else{
                if(counter > 0)
                    values.push_back(condition);

                condition = m_where->is_true(rule, row.value(table->get_column(rule.column)->second)).value();
                counter = 1;
            }
        }

        if(counter > 0)
            values.push_back(condition);

        for(const auto& val : values){
            result += val;
        }

        return result;
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
                values.push_back(TRY(column->evaluate(context, row)));
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
        for (size_t i = 0; i < m_columns.columns().size(); i++){
            if(m_columns.aliases()[i].has_value())
                column_names.push_back(m_columns.aliases()[i].value());
            else
                column_names.push_back(m_columns.columns()[i]->to_string());
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
