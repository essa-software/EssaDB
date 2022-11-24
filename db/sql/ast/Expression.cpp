#include "Expression.hpp"
#include "db/sql/SQLError.hpp"

#include <cstddef>
#include <db/core/Column.hpp>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>
#include <db/core/Regex.hpp>
#include <db/core/Table.hpp>
#include <db/core/Tuple.hpp>
#include <db/core/Value.hpp>
#include <db/sql/ast/TableExpression.hpp>
#include <map>
#include <memory>
#include <vector>

namespace Db::Sql::AST {

SQLErrorOr<Core::Value> Check::evaluate(EvaluationContext& context) const {
    if (TRY(TRY(m_main_check->evaluate(context)).to_bool().map_error(DbToSQLError { start() }))) {
        return Core::Value::create_bool(false);
    }

    for (const auto& check : m_constraints) {
        if (TRY(TRY(check.second->evaluate(context)).to_bool().map_error(DbToSQLError { start() }))) {
            return Core::Value::create_bool(false);
        }
    }

    return Core::Value::create_bool(true);
}

SQLErrorOr<void> Check::add_check(std::shared_ptr<AST::Expression> expr) {
    if (m_main_check)
        return SQLError { "Check already exists", start() };

    m_main_check = std::move(expr);

    return {};
}

SQLErrorOr<void> Check::alter_check(std::shared_ptr<AST::Expression> expr) {
    if (!m_main_check)
        return SQLError { "No check to alter!", start() };

    m_main_check = std::move(expr);

    return {};
}

SQLErrorOr<void> Check::drop_check() {
    if (!m_main_check)
        return SQLError { "No check to drop!", start() };

    delete m_main_check.get();
    m_main_check = nullptr;

    return {};
}

SQLErrorOr<void> Check::add_constraint(const std::string& name, std::shared_ptr<AST::Expression> expr) {
    auto constraint = m_constraints.find(name);

    if (constraint != m_constraints.end())
        return SQLError { "Constraint with name " + name + " already exists", start() };

    m_constraints.insert({ name, std::move(expr) });

    return {};
}

SQLErrorOr<void> Check::alter_constraint(const std::string& name, std::shared_ptr<AST::Expression> expr) {
    auto constraint = m_constraints.find(name);

    if (constraint == m_constraints.end())
        return SQLError { "Constraint with name " + name + " not found", start() };

    constraint->second = std::move(expr);

    return {};
}

SQLErrorOr<void> Check::drop_constraint(const std::string& name) {
    auto constraint = m_constraints.find(name);

    if (constraint == m_constraints.end())
        return SQLError { "Constraint with name " + name + " not found", start() };

    m_constraints.erase(name);

    return {};
}

SQLErrorOr<Core::Value> Identifier::evaluate(EvaluationContext& context) const {
    if (!context.db) {
        return SQLError { "Identifiers cannot be resolved without database", start() };
    }

    if (context.current_frame().row_type == EvaluationContextFrame::RowType::FromTable) {
        std::optional<size_t> index;
        Core::TupleWithSource const* row = nullptr;
        for (auto it = context.frames.rbegin(); it != context.frames.rend(); it++) {
            auto const& frame = *it;
            if (!frame.table) {
                return SQLError { "Identifiers cannot be resolved without table", start() };
            }
            index = TRY(frame.table->resolve_identifier(context.db, *this));
            if (index) {
                row = &frame.row;
                break;
            }
        }
        if (!index) {
            return SQLError { "Invalid identifier", start() };
        }
        return row->tuple.value(*index);
    }

    return TRY(context.current_frame().columns.resolve_value(context, *this));
}

// FIXME: char ranges doesn't work in row
static Core::DbErrorOr<bool> wildcard_parser(std::string const& lhs, std::string const& rhs) {
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

SQLErrorOr<bool> BinaryOperator::is_true(EvaluationContext& context) const {
    // TODO: Implement proper comparison
    switch (m_operation) {
    case Operation::Equal:
        return (TRY(m_lhs->evaluate(context)) == TRY(m_rhs->evaluate(context)))
            .map_error(DbToSQLError { start() });
    case Operation::NotEqual:
        return (TRY(m_lhs->evaluate(context)) != TRY(m_rhs->evaluate(context)))
            .map_error(DbToSQLError { start() });
    case Operation::Greater:
        return (TRY(m_lhs->evaluate(context)) > TRY(m_rhs->evaluate(context)))
            .map_error(DbToSQLError { start() });
    case Operation::GreaterEqual:
        return (TRY(m_lhs->evaluate(context)) >= TRY(m_rhs->evaluate(context)))
            .map_error(DbToSQLError { start() });
    case Operation::Less:
        return (TRY(m_lhs->evaluate(context)) < TRY(m_rhs->evaluate(context)))
            .map_error(DbToSQLError { start() });
    case Operation::LessEqual:
        return (TRY(m_lhs->evaluate(context)) <= TRY(m_rhs->evaluate(context)))
            .map_error(DbToSQLError { start() });
    case Operation::And:
        return (TRY(TRY(m_lhs->evaluate(context)).to_bool().map_error(DbToSQLError { start() }))
            && TRY(TRY(m_rhs->evaluate(context)).to_bool().map_error(DbToSQLError { start() })));
    case Operation::Or:
        return (TRY(TRY(m_lhs->evaluate(context)).to_bool().map_error(DbToSQLError { start() }))
            || TRY(TRY(m_rhs->evaluate(context)).to_bool().map_error(DbToSQLError { start() })));
    case Operation::Not:
        return (TRY(TRY(m_lhs->evaluate(context)).to_bool().map_error(DbToSQLError { start() })));
    case Operation::Like: {
        return wildcard_parser(TRY(TRY(m_lhs->evaluate(context)).to_string().map_error(DbToSQLError { start() })),
            TRY(TRY(m_rhs->evaluate(context)).to_string().map_error(DbToSQLError { start() })))
            .map_error(DbToSQLError { start() });
    }
    case Operation::Match: {
        auto value = TRY(TRY(m_lhs->evaluate(context)).to_string().map_error(DbToSQLError { start() }));
        auto pattern = TRY(TRY(m_rhs->evaluate(context)).to_string().map_error(DbToSQLError { start() }));
        try {
            std::regex regex { pattern };
            return std::regex_match(value, regex);
        } catch (std::regex_error const& error) {
            return SQLError { error.what(), start() };
        }
    }
    case Operation::Invalid:
        break;
    }
    __builtin_unreachable();
}

SQLErrorOr<Core::Value> ArithmeticOperator::evaluate(EvaluationContext& context) const {
    auto lhs = TRY(m_lhs->evaluate(context));
    auto rhs = TRY(m_rhs->evaluate(context));

    switch (m_operation) {
    case Operation::Add:
        return (lhs + rhs).map_error(DbToSQLError { start() });
    case Operation::Sub:
        return (lhs - rhs).map_error(DbToSQLError { start() });
    case Operation::Mul:
        return (lhs * rhs).map_error(DbToSQLError { start() });
    case Operation::Div:
        return (lhs / rhs).map_error(DbToSQLError { start() });
    case Operation::Invalid:
        break;
    }
    __builtin_unreachable();
}

std::string ArithmeticOperator::to_string() const {
    std::string string;
    string += "(" + m_lhs->to_string();

    switch (m_operation) {
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

SQLErrorOr<Core::Value> UnaryOperator::evaluate(EvaluationContext& context) const {
    switch (m_operation) {
    case Operation::Minus:
        return (Core::Value::create_int(0) - TRY(m_operand->evaluate(context))).map_error(DbToSQLError { start() });
    }
    ESSA_UNREACHABLE;
}

SQLErrorOr<Core::Value> BetweenExpression::evaluate(EvaluationContext& context) const {
    // TODO: Implement this for strings etc
    auto value = TRY(m_lhs->evaluate(context));
    auto min = TRY(m_min->evaluate(context));
    auto max = TRY(m_max->evaluate(context));
    return Core::Value::create_bool(TRY((value >= min).map_error(DbToSQLError { start() }))
        && TRY((value <= max).map_error(DbToSQLError { start() })));
}

SQLErrorOr<Core::Value> InExpression::evaluate(EvaluationContext& context) const {
    // TODO: Implement this for strings etc
    auto value = TRY(TRY(m_lhs->evaluate(context)).to_string().map_error(DbToSQLError { start() }));
    for (const auto& arg : m_args) {
        auto to_compare = TRY(TRY(arg->evaluate(context)).to_string().map_error(DbToSQLError { start() }));

        if (value == to_compare)
            return Core::Value::create_bool(true);
    }
    return Core::Value::create_bool(false);
}

SQLErrorOr<Core::Value> IsExpression::evaluate(EvaluationContext& context) const {
    auto lhs = TRY(m_lhs->evaluate(context));
    switch (m_what) {
    case What::Null:
        return Core::Value::create_bool(lhs.is_null());
    case What::NotNull:
        return Core::Value::create_bool(!lhs.is_null());
    }
    __builtin_unreachable();
}

SQLErrorOr<Core::Value> CaseExpression::evaluate(EvaluationContext& context) const {
    for (const auto& case_expression : m_cases) {
        if (TRY(TRY(case_expression.expr->evaluate(context)).to_bool().map_error(DbToSQLError { start() })))
            return TRY(case_expression.value->evaluate(context));
    }

    if (m_else_value)
        return TRY(m_else_value->evaluate(context));
    return Core::Value::null();
}

SQLErrorOr<Core::Value> NonOwningExpressionProxy::evaluate(EvaluationContext& context) const {
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

SQLErrorOr<Core::Value> IndexExpression::evaluate(EvaluationContext& context) const {
    auto const& tuple = context.current_frame().row.tuple;
    if (m_index >= tuple.value_count()) {
        return SQLError { "Internal error: index expression overflow", start() };
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
