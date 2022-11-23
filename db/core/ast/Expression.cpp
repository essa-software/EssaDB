#include "Expression.hpp"

#include <cstddef>
#include <db/core/Column.hpp>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>
#include <db/core/Regex.hpp>
#include <db/core/Table.hpp>
#include <db/core/Tuple.hpp>
#include <db/core/Value.hpp>
#include <db/core/ast/TableExpression.hpp>
#include <map>
#include <memory>
#include <vector>

namespace Db::Core::AST {

DbErrorOr<Value> Check::evaluate(EvaluationContext& context) const {
    if (TRY(TRY(m_main_check->evaluate(context)).to_bool())) {
        return Value::create_bool(false);
    }

    for (const auto& check : m_constraints) {
        if (TRY(TRY(check.second->evaluate(context)).to_bool())) {
            return Value::create_bool(false);
        }
    }

    return Value::create_bool(true);
}

DbErrorOr<void> Check::add_check(std::shared_ptr<AST::Expression> expr) {
    if (m_main_check)
        DbError { "Check already exists", 0 };

    m_main_check = std::move(expr);

    return {};
}

DbErrorOr<void> Check::alter_check(std::shared_ptr<AST::Expression> expr) {
    if (!m_main_check)
        DbError { "No check to alter!", 0 };

    m_main_check = std::move(expr);

    return {};
}

DbErrorOr<void> Check::drop_check() {
    if (!m_main_check)
        DbError { "No check to drop!", 0 };

    delete m_main_check.get();
    m_main_check = nullptr;

    return {};
}

DbErrorOr<void> Check::add_constraint(const std::string& name, std::shared_ptr<AST::Expression> expr) {
    auto constraint = m_constraints.find(name);

    if (constraint != m_constraints.end())
        DbError { "Constraint with name " + name + " already exists", 0 };

    m_constraints.insert({ name, std::move(expr) });

    return {};
}

DbErrorOr<void> Check::alter_constraint(const std::string& name, std::shared_ptr<AST::Expression> expr) {
    auto constraint = m_constraints.find(name);

    if (constraint == m_constraints.end())
        DbError { "Constraint with name " + name + " not found", 0 };

    constraint->second = std::move(expr);

    return {};
}

DbErrorOr<void> Check::drop_constraint(const std::string& name) {
    auto constraint = m_constraints.find(name);

    if (constraint == m_constraints.end())
        DbError { "Constraint with name " + name + " not found", 0 };

    m_constraints.erase(name);

    return {};
}

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context) const {
    if (!context.db) {
        return DbError { "Identifiers cannot be resolved without database", start() };
    }

    if (context.current_frame().row_type == EvaluationContextFrame::RowType::FromTable) {
        std::optional<size_t> index;
        TupleWithSource const* row = nullptr;
        for (auto it = context.frames.rbegin(); it != context.frames.rend(); it++) {
            auto const& frame = *it;
            if (!frame.table) {
                return DbError { "Identifiers cannot be resolved without table", start() };
            }
            index = TRY(frame.table->resolve_identifier(context.db, *this));
            if (index) {
                row = &frame.row;
                break;
            }
        }
        if (!index) {
            return DbError { "Invalid identifier", start() };
        }
        return row->tuple.value(*index);
    }

    return TRY(context.current_frame().columns.resolve_value(context, *this));
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

DbErrorOr<bool> BinaryOperator::is_true(EvaluationContext& context) const {
    // TODO: Implement proper comparison
    switch (m_operation) {
    case Operation::Equal:
        return TRY(m_lhs->evaluate(context)) == TRY(m_rhs->evaluate(context));
    case Operation::NotEqual:
        return TRY(m_lhs->evaluate(context)) != TRY(m_rhs->evaluate(context));
    case Operation::Greater:
        return TRY(m_lhs->evaluate(context)) > TRY(m_rhs->evaluate(context));
    case Operation::GreaterEqual:
        return TRY(m_lhs->evaluate(context)) >= TRY(m_rhs->evaluate(context));
    case Operation::Less:
        return TRY(m_lhs->evaluate(context)) < TRY(m_rhs->evaluate(context));
    case Operation::LessEqual:
        return TRY(m_lhs->evaluate(context)) <= TRY(m_rhs->evaluate(context));
    case Operation::And:
        return TRY(TRY(m_lhs->evaluate(context)).to_bool()) && TRY(TRY(m_rhs->evaluate(context)).to_bool());
    case Operation::Or:
        return TRY(TRY(m_lhs->evaluate(context)).to_bool()) || TRY(TRY(m_rhs->evaluate(context)).to_bool());
    case Operation::Not:
        return TRY(TRY(m_lhs->evaluate(context)).to_bool());
    case Operation::Like: {
        return wildcard_parser(TRY(TRY(m_lhs->evaluate(context)).to_string()), TRY(TRY(m_rhs->evaluate(context)).to_string()));
    }
    case Operation::Match: {
        auto value = TRY(TRY(m_lhs->evaluate(context)).to_string());
        auto pattern = TRY(TRY(m_rhs->evaluate(context)).to_string());
        try {
            std::regex regex { pattern };
            return std::regex_match(value, regex);
        } catch (std::regex_error const& error) {
            return DbError { error.what(), start() };
        }
    }
    case Operation::Invalid:
        break;
    }
    __builtin_unreachable();
}

DbErrorOr<Value> ArithmeticOperator::evaluate(EvaluationContext& context) const {
    auto lhs = TRY(m_lhs->evaluate(context));
    auto rhs = TRY(m_rhs->evaluate(context));

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

std::string ArithmeticOperator::to_string() const {
    std::string string;
    string += "(" + m_lhs->to_string();

    switch(m_operation) {
        case Operation::Add:
            string += " + ";
            break;
        case Operation::Sub:
            string += " - ";
            break;
        case Operation::Mul:
            string += " * ";
            break;
        case Operation::Div:
            string += " / ";
            break;
        case Operation::Invalid:
            string += " AO? ";
            break;
    }

    string += m_rhs->to_string() + ")";
    return string;
}

DbErrorOr<Value> UnaryOperator::evaluate(EvaluationContext& context) const {
    switch (m_operation) {
    case Operation::Minus:
        return Value::create_int(0) - TRY(m_operand->evaluate(context));
    }
    ESSA_UNREACHABLE;
}

DbErrorOr<Value> BetweenExpression::evaluate(EvaluationContext& context) const {
    // TODO: Implement this for strings etc
    auto value = TRY(m_lhs->evaluate(context));
    auto min = TRY(m_min->evaluate(context));
    auto max = TRY(m_max->evaluate(context));
    return Value::create_bool(TRY(value >= min) && TRY(value <= max));
}

DbErrorOr<Value> InExpression::evaluate(EvaluationContext& context) const {
    // TODO: Implement this for strings etc
    auto value = TRY(TRY(m_lhs->evaluate(context)).to_string());
    for (const auto& arg : m_args) {
        auto to_compare = TRY(TRY(arg->evaluate(context)).to_string());

        if (value == to_compare)
            return Value::create_bool(true);
    }
    return Value::create_bool(false);
}

DbErrorOr<Value> IsExpression::evaluate(EvaluationContext& context) const {
    auto lhs = TRY(m_lhs->evaluate(context));
    switch (m_what) {
    case What::Null:
        return Value::create_bool(lhs.is_null());
    case What::NotNull:
        return Value::create_bool(!lhs.is_null());
    }
    __builtin_unreachable();
}

DbErrorOr<Value> CaseExpression::evaluate(EvaluationContext& context) const {
    for (const auto& case_expression : m_cases) {
        if (TRY(TRY(case_expression.expr->evaluate(context)).to_bool()))
            return TRY(case_expression.value->evaluate(context));
    }

    if (m_else_value)
        return TRY(m_else_value->evaluate(context));
    return Value::null();
}

DbErrorOr<Value> NonOwningExpressionProxy::evaluate(EvaluationContext& context) const {
    return m_expression.evaluate(context);
}

std::string NonOwningExpressionProxy::to_string() const {
    return m_expression.to_string();
}

std::vector<std::string> NonOwningExpressionProxy::referenced_columns() const {
    return m_expression.referenced_columns();
}

bool NonOwningExpressionProxy::contains_aggregate_function() const {
    return m_expression.contains_aggregate_function();
}

DbErrorOr<Value> IndexExpression::evaluate(EvaluationContext& context) const {
    auto const& tuple = context.current_frame().row.tuple;
    if (m_index >= tuple.value_count()) {
        return DbError { "Internal error: index expression overflow", start() };
    }
    return tuple.value(m_index);
}

std::string IndexExpression::to_string() const {
    return m_name;
}

std::vector<std::string> IndexExpression::referenced_columns() const {
    return {};
}

}
