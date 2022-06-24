#include "Expression.hpp"
#include "Database.hpp"
#include "Table.hpp"

namespace Db::Core::AST {

DbErrorOr<Value> Expression::evaluate_and_require_single_value(EvaluationContext& context, TupleWithSource const& row, std::string run_from) const {
    auto result = TRY(evaluate(context, row));
    if (result.type() != Value::Type::SelectResult)
        return result;
    auto select_result = TRY(result.to_select_result());
    if (!select_result.is_convertible_to_value())
        return DbError { "Expression result must contain 1 row and 1 column " + (run_from.empty() ? "here" : "in " + run_from), start() };
    return select_result.as_value();
}

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    if (context.row_type == EvaluationContext::RowType::FromTable) {
        size_t index = 0;
        if (m_table) {
            auto table = TRY(context.db->table(*m_table));

            auto column = table->get_column(m_id);
            if (!column)
                return DbError { "No such column: " + m_id, start() };
            index = column->index;
        }
        else if (context.table) {
            auto column = context.table->get_column(m_id);
            if (!column)
                return DbError { "No such column: " + m_id, start() };
            index = column->index;
        }
        else {
            return DbError { "You need a table to resolve identifiers", start() };
        }
        return row.tuple.value(index);
    }

    return TRY(context.columns.resolve_value(context, row, m_id));
}

// FIXME: char ranges doesn't work in row
static DbErrorOr<bool> wildcard_parser(std::string const& lhs, std::string const& rhs) {
    auto is_string_valid = [](std::string const& lhs, std::string const& rhs) {
        size_t len = 0;

        for (auto left_it = lhs.begin(), right_it = rhs.begin(); left_it < lhs.end() || right_it < rhs.end(); left_it++, right_it++) {
            len++;
            if (*right_it == '?') {
                if (lhs.size() < len)
                    return false;
                continue;
            }
            else if (*right_it == '#') {
                if (!isdigit(*left_it) || lhs.size() < len)
                    return false;
            }
            else if (*right_it == '[') {
                if (lhs.size() < len)
                    return false;

                right_it++;
                bool negate = 0;
                if (*right_it == '!') {
                    negate = 1;
                    right_it++;
                }

                std::vector<char> allowed_chars;

                for (auto it = right_it; *it != ']'; it++) {
                    allowed_chars.push_back(*it);
                    right_it++;
                }
                right_it++;

                if (allowed_chars.size() == 3 && allowed_chars[1] == '-') {
                    bool in_range = (allowed_chars[0] <= *left_it && *left_it <= allowed_chars[2]);
                    if (negate ? in_range : !in_range)
                        return false;
                }
                else {
                    bool exists = 0;
                    for (const auto& c : allowed_chars) {
                        exists |= (c == *left_it);
                    }

                    if (negate ? exists : !exists)
                        return false;
                }
            }
            else if (*right_it != *left_it) {
                return false;
            }
        }

        return true;
    };

    bool result = 0;
    auto left_it = lhs.begin(), right_it = rhs.begin();

    for (left_it = lhs.begin(), right_it = rhs.begin(); left_it < lhs.end(); left_it++) {
        size_t dist_to_next_asterisk = 0, substr_len = 0;
        bool brackets = false;
        if (*right_it == '*')
            right_it++;

        for (auto it = right_it; it < rhs.end() && *it != '*'; it++) {
            dist_to_next_asterisk++;

            if (brackets && *it != ']')
                continue;

            substr_len++;

            if (*it == '[')
                brackets = true;
        }

        size_t lstart = static_cast<size_t>(left_it - lhs.begin());
        size_t rstart = static_cast<size_t>(right_it - rhs.begin());
        try {
            if (is_string_valid(lhs.substr(lstart, substr_len), rhs.substr(rstart, dist_to_next_asterisk))) {
                left_it += substr_len;
                right_it += dist_to_next_asterisk;

                result = 1;
            }
        } catch (...) {
            return false;
        }
    }
    if (right_it != rhs.end() && rhs.back() != '*')
        return false;
    return result;
}

DbErrorOr<bool> BinaryOperator::is_true(EvaluationContext& context, TupleWithSource const& row) const {
    // TODO: Implement proper comparison
    switch (m_operation) {
    case Operation::Equal:
        return TRY(m_lhs->evaluate(context, row)) == TRY(m_rhs->evaluate(context, row));
    case Operation::NotEqual:
        return TRY(m_lhs->evaluate(context, row)) != TRY(m_rhs->evaluate(context, row));
    case Operation::Greater:
        return TRY(m_lhs->evaluate(context, row)) > TRY(m_rhs->evaluate(context, row));
    case Operation::GreaterEqual:
        return TRY(m_lhs->evaluate(context, row)) >= TRY(m_rhs->evaluate(context, row));
    case Operation::Less:
        return TRY(m_lhs->evaluate(context, row)) < TRY(m_rhs->evaluate(context, row));
    case Operation::LessEqual:
        return TRY(m_lhs->evaluate(context, row)) <= TRY(m_rhs->evaluate(context, row));
    case Operation::And:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_bool()) && TRY(TRY(m_rhs->evaluate(context, row)).to_bool());
    case Operation::Or:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_bool()) || TRY(TRY(m_rhs->evaluate(context, row)).to_bool());
    case Operation::Not:
        return TRY(TRY(m_lhs->evaluate(context, row)).to_bool());
    case Operation::Like: {
        return wildcard_parser(TRY(TRY(m_lhs->evaluate(context, row)).to_string()), TRY(TRY(m_rhs->evaluate(context, row)).to_string()));
    }
    case Operation::Invalid:
        break;
    }
    __builtin_unreachable();
}

DbErrorOr<Value> ArithmeticOperator::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    auto lhs = TRY(m_lhs->evaluate(context, row));
    auto rhs = TRY(m_rhs->evaluate(context, row));

    switch (m_operation) {
    case Operation::Add:
        return lhs + rhs;
    case Operation::Sub:
        return lhs - rhs;
    case Operation::Mul:
        return lhs * rhs;
    case Operation::Div:
        return lhs / rhs;
    case Operation::Invalid:
        break;
    }
    __builtin_unreachable();
}

DbErrorOr<Value> BetweenExpression::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    // TODO: Implement this for strings etc
    auto value = TRY(TRY(m_lhs->evaluate(context, row)).to_int());
    auto min = TRY(TRY(m_min->evaluate(context, row)).to_int());
    auto max = TRY(TRY(m_max->evaluate(context, row)).to_int());
    return Value::create_bool(value >= min && value <= max);
}

DbErrorOr<Value> InExpression::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    // TODO: Implement this for strings etc
    auto value = TRY(TRY(m_lhs->evaluate(context, row)).to_string());
    for (const auto& arg : m_args) {
        auto to_compare = TRY(TRY(arg->evaluate(context, row)).to_string());

        if (value == to_compare)
            return Value::create_bool(true);
    }
    return Value::create_bool(false);
}

DbErrorOr<Value> IsExpression::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    auto lhs = TRY(m_lhs->evaluate(context, row));
    switch (m_what) {
    case What::Null:
        return Value::create_bool(lhs.is_null());
    case What::NotNull:
        return Value::create_bool(!lhs.is_null());
    }
    __builtin_unreachable();
}

DbErrorOr<Value> CaseExpression::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    for (const auto& case_expression : m_cases) {
        if (TRY(TRY(case_expression.expr->evaluate(context, row)).to_bool()))
            return TRY(case_expression.value->evaluate(context, row));
    }

    if (m_else_value)
        return TRY(m_else_value->evaluate(context, row));
    return Value::null();
}

SelectColumns::SelectColumns(std::vector<Column> columns)
    : m_columns(std::move(columns)) {
    size_t index = 0;
    for (auto const& column : m_columns) {
        if (column.alias)
            m_aliases.insert({ *column.alias, ResolvedAlias { .column = *column.column, .index = index } });
        m_aliases.insert({ column.column->to_string(), ResolvedAlias { .column = *column.column, .index = index } });
        index++;
    }
}

SelectColumns::ResolvedAlias const* SelectColumns::resolve_alias(std::string const& alias) const {
    auto it = m_aliases.find(alias);
    if (it == m_aliases.end()) {
        return nullptr;
    }
    return &it->second;
}

DbErrorOr<Value> SelectColumns::resolve_value(EvaluationContext& context, TupleWithSource const& tuple, std::string const& alias) const {
    auto resolved_alias = resolve_alias(alias);
    if (resolved_alias)
        return tuple.tuple.value(resolved_alias->index);

    if (!context.table)
        return DbError { "Identifier '" + alias + "' not defined", 0 };

    auto column = context.table->get_column(alias);
    if (!column)
        return DbError { "Identifier '" + alias + "' not defined in table nor as an alias", 0 };
    if (!tuple.source)
        return DbError { "Cannot use table columns on aggregated rows", 0 };
    return tuple.source->value(column->index);
}

DbErrorOr<Value> ExpressionOrIndex::evaluate(EvaluationContext& context, TupleWithSource const& input) const {
    if (is_expression()) {
        return TRY(expression().evaluate(context, input));
    }
    auto index = this->index();
    if (index >= context.columns.columns().size()) {
        // TODO: Store location info
        return DbError { "Index out of range", 0 };
    }
    return TRY(context.columns.columns()[index].column->evaluate(context, input));
}
}
