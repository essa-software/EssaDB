#include "Expression.hpp"

#include "Column.hpp"
#include "Database.hpp"
#include "DbError.hpp"
#include "RowWithColumnNames.hpp"
#include "Table.hpp"
#include "Tuple.hpp"
#include "Value.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <regex>
#include <vector>

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

DbErrorOr<Value> Check::evaluate(EvaluationContext& context, const TupleWithSource& row) const {
    if (TRY(TRY(m_main_check->evaluate(context, row)).to_bool())) {
        return Value::create_bool(false);
    }

    for (const auto& check : m_constraints) {
        if (TRY(TRY(check.second->evaluate(context, row)).to_bool())) {
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

DbErrorOr<std::unique_ptr<Table>> TableIdentifier::evaluate(Database* db) const {
    auto table_ptr = TRY(db->table(m_id));
    MemoryBackedTable table(nullptr, table_ptr->name());

    for (const auto& column : table_ptr->columns()) {
        TRY(table.add_column(column));
    }

    for (const auto& row : table_ptr->raw_rows()) {
        TRY(table.insert(row));
    }

    return std::make_unique<MemoryBackedTable>(std::move(table));
}

Tuple TableExpression::prepare_tuple(const Tuple* lhs_row, const Tuple* rhs_row, bool order) {
    std::vector<Value> row;
    if (order) {
        if (lhs_row) {
            for (size_t i = 0; i < lhs_row->value_count(); i++) {
                row.push_back(lhs_row->value(i));
            }
        }

        if (rhs_row) {
            for (size_t i = 0; i < rhs_row->value_count(); i++) {
                row.push_back(rhs_row->value(i));
            }
        }
    }
    else {
        if (rhs_row) {
            for (size_t i = 0; i < rhs_row->value_count(); i++) {
                row.push_back(rhs_row->value(i));
            }
        }

        if (lhs_row) {
            for (size_t i = 0; i < lhs_row->value_count(); i++) {
                row.push_back(lhs_row->value(i));
            }
        }
    }

    return Tuple(row);
}

DbErrorOr<std::unique_ptr<Table>> JoinExpression::evaluate(Database* db) const {
    auto table = std::make_unique<MemoryBackedTable>(nullptr, "");

    auto lhs_ptr = TRY(m_lhs->evaluate(db));
    auto rhs_ptr = TRY(m_rhs->evaluate(db));

    auto lhs = dynamic_cast<MemoryBackedTable*>(lhs_ptr.get());
    auto rhs = dynamic_cast<MemoryBackedTable*>(rhs_ptr.get());

    std::multimap<Value, std::pair<MemoryBackedTable*, Tuple>, ValueSorter> contents;

    size_t lhs_index = 0, rhs_index = 0;
    bool add_to_index = true;

    for (const auto& column : lhs->columns()) {
        if (column.name() == m_on_lhs->referenced_columns().front())
            add_to_index = false;
        if (add_to_index)
            lhs_index++;
        std::string name = (column.original_table()->name().empty() ? "" : column.original_table()->name() + ".") + column.name();
        TRY(table->add_column(Column(name, column.type(), 0, 0, 0)));
    }

    add_to_index = true;

    for (const auto& column : rhs->columns()) {
        if (column.name() == m_on_rhs->referenced_columns().front())
            add_to_index = false;
        if (add_to_index)
            rhs_index++;
        std::string name = (column.original_table()->name().empty() ? "" : column.original_table()->name() + ".") + column.name();
        TRY(table->add_column(Column(name, column.type(), 0, 0, 0)));
    }

    for (const auto& row : *lhs) {
        contents.insert(std::pair(row.value(lhs_index), std::make_pair(lhs, row)));
    }

    for (const auto& row : *rhs) {
        contents.insert(std::pair(row.value(rhs_index), std::make_pair(rhs, row)));
    }

    auto beg = contents.begin();
    beg++;
    auto last = contents.begin();

    switch (m_join_type) {
    case Type::InnerJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs && it->second.first == rhs) {
                if (TRY(it->first == last->first)) {
                    auto row = prepare_tuple(&last->second.second, &it->second.second, 1);
                    TRY(table->insert(row));
                    it++;
                    if (it == contents.end())
                        break;
                }
            }
            last = it;
        }

        break;
    }

    case Type::LeftJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs) {
                if (TRY(it->first == last->first)) {
                    auto row = prepare_tuple(&last->second.second, &it->second.second, 1);
                    TRY(table->insert(row));
                    it++;
                    if (it == contents.end())
                        break;
                }
                else {
                    std::vector<Value> values(rhs->columns().size(), Value::null());
                    Tuple dummy(values);

                    auto row = prepare_tuple(&last->second.second, &dummy, 1);

                    TRY(table->insert(row));
                }
            }
            last = it;
        }

        break;
    }

    case Type::RightJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == rhs || it->second.first == rhs) {
                if (TRY(it->first == last->first)) {
                    auto row = prepare_tuple(&last->second.second, &it->second.second, 0);
                    TRY(table->insert(row));
                    it++;
                    if (it == contents.end())
                        break;

                    last = it;
                }
                else {
                    std::vector<Value> values(lhs->columns().size(), Value::null());
                    Tuple dummy(values);

                    auto row = prepare_tuple(&dummy, &it->second.second, 0);

                    TRY(table->insert(row));
                }
            }
            last = it;
        }

        break;
    }

    case Type::OuterJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs && it->second.first == rhs && TRY(it->first == last->first)) {
                auto row = prepare_tuple(&last->second.second, &it->second.second, 1);
                TRY(table->insert(row));
                it++;
                if (it == contents.end())
                    break;
            }
            else if (last->second.first == lhs) {
                std::vector<Value> values(rhs->columns().size(), Value::null());
                Tuple dummy(values);

                auto row = prepare_tuple(&last->second.second, &dummy, 1);

                TRY(table->insert(row));
            }
            else if (last->second.first == rhs) {
                std::vector<Value> values(lhs->columns().size(), Value::null());
                Tuple dummy(values);

                auto row = prepare_tuple(&dummy, &it->second.second, 0);

                TRY(table->insert(row));
            }
            last = it;
        }

        break;
    }

    case Type::Invalid:
        return DbError { "Invalid join type!", start() };
        break;
    }

    return table;
}

DbErrorOr<std::unique_ptr<Table>> CrossJoinExpression::evaluate(Database* db) const {
    auto table = std::make_unique<MemoryBackedTable>(nullptr, "");

    auto lhs_ptr = TRY(m_lhs->evaluate(db));
    auto rhs_ptr = TRY(m_rhs->evaluate(db));

    auto lhs = dynamic_cast<MemoryBackedTable*>(lhs_ptr.get());
    auto rhs = dynamic_cast<MemoryBackedTable*>(rhs_ptr.get());

    for (const auto& column : lhs->columns()) {
        std::string name = (column.original_table()->name().empty() ? "" : column.original_table()->name() + ".") + column.name();
        TRY(table->add_column(Column(name, column.type(), 0, 0, 0)));
    }

    for (const auto& column : rhs->columns()) {
        std::string name = (column.original_table()->name().empty() ? "" : column.original_table()->name() + ".") + column.name();
        TRY(table->add_column(Column(name, column.type(), 0, 0, 0)));
    }

    for (const auto& lhs_row : lhs->raw_rows()) {
        for (const auto& rhs_row : rhs->raw_rows()) {
            auto row = prepare_tuple(&lhs_row, &rhs_row, 1);
            TRY(table->insert(row));
        }
    }

    return table;
}

DbErrorOr<Value> Identifier::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
    if (context.row_type == EvaluationContext::RowType::FromTable) {
        size_t index = 0;
        if (context.table) {
            if (m_table) {
                auto table = TRY(context.db->table(*m_table));

                auto column = context.table->get_column(m_id, table);
                if (!column)
                    return DbError { "No such column: " + m_id, start() };
                index = column->index;
            }
            else {
                auto column = context.table->get_column(m_id);
                if (!column)
                    return DbError { "No such column: " + m_id, start() };
                index = column->index;
            }
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
    case Operation::Match: {
        auto value = TRY(TRY(m_lhs->evaluate(context, row)).to_string());
        auto pattern = TRY(TRY(m_rhs->evaluate(context, row)).to_string());
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
