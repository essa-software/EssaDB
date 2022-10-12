#include "Select.hpp"
#include "Database.hpp"
#include "DbError.hpp"
#include "Expression.hpp"
#include "Function.hpp"
#include "Table.hpp"
#include "Value.hpp"
#include <EssaUtil/Is.hpp>
#include <EssaUtil/ScopeGuard.hpp>
#include <cstddef>
#include <memory>

namespace Db::Core::AST {

DbErrorOr<ResultSet> Select::execute(EvaluationContext& context) const {
    // Comments specify SQL Conceptional Evaluation:
    // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql#logical-processing-order-of-the-select-statement
    // FROM

    std::unique_ptr<Table> table = m_options.from ? TRY(m_options.from->evaluate(context)) : nullptr;

    SelectColumns select_all_columns;

    SelectColumns const& columns = *TRY([this, table = table.get(), &select_all_columns]() -> DbErrorOr<SelectColumns const*> {
        if (m_options.columns.select_all()) {
            if (!table) {
                return DbError { "You need a table to do SELECT *", m_start };
            }
            std::vector<SelectColumns::Column> all_columns;
            for (size_t s = 0; s < table->columns().size(); s++) {
                all_columns.push_back(SelectColumns::Column { .column = std::make_unique<IndexExpression>(m_start + 1, s, table->columns()[s].name()) });
            }
            select_all_columns = SelectColumns { std::move(all_columns) };
            return &select_all_columns;
        }
        return &m_options.columns;
    }());

    auto& frame = context.frames.emplace_back(m_options.from.get(), columns);
    Util::ScopeGuard guard { [&] { context.frames.pop_back(); } };

    auto rows = TRY([&]() -> DbErrorOr<std::vector<TupleWithSource>> {
        if (m_options.from) {
            // SELECT etc.
            // TODO: Make use of iterator capabilities of this instead of
            //       reading everything into memory.
            return collect_rows(context, *table);
        }

        std::vector<Value> values;
        for (auto const& column : m_options.columns.columns()) {
            values.push_back(TRY(column.column->evaluate(context)));
        }
        return std::vector<TupleWithSource> { { .tuple = Tuple { values }, .source = {} } };
    }());

    frame.row_type = EvaluationContextFrame::RowType::FromResultSet;

    // DISTINCT
    if (m_options.distinct) {
        std::vector<TupleWithSource> occurences;

        // FIXME: O(n^2)
        for (const auto& row : rows) {
            bool distinct = true;
            for (const auto& to_compare : occurences) {
                if (TRY(row == to_compare)) {
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
    if (m_options.order_by) {
        auto generate_tuple_pair_for_ordering = [&](TupleWithSource const& lhs, TupleWithSource const& rhs) -> DbErrorOr<std::pair<Tuple, Tuple>> {
            std::vector<Value> lhs_values;
            std::vector<Value> rhs_values;

            for (auto const& column : m_options.order_by->columns) {
                frame.row = lhs;
                auto lhs_value = TRY(column.expression->evaluate(context));
                frame.row = rhs;
                auto rhs_value = TRY(column.expression->evaluate(context));

                if (column.order == OrderBy::Order::Ascending) {
                    lhs_values.push_back(std::move(lhs_value));
                    rhs_values.push_back(std::move(rhs_value));
                }
                else {
                    lhs_values.push_back(std::move(rhs_value));
                    rhs_values.push_back(std::move(lhs_value));
                }
            }

            return std::pair<Tuple, Tuple> { Tuple { lhs_values }, Tuple { rhs_values } };
        };

        std::optional<DbError> error;
        std::stable_sort(rows.begin(), rows.end(), [&](TupleWithSource const& lhs, TupleWithSource const& rhs) -> bool {
            auto pair = generate_tuple_pair_for_ordering(lhs, rhs);
            if (pair.is_error()) {
                error = pair.release_error();
                return false;
            }
            auto [lhs_value, rhs_value] = pair.release_value();
            // std::cout << lhs_value << " < " << rhs_value << std::endl;
            return lhs_value < rhs_value;
        });
        if (error)
            return *error;
    }

    if (m_options.top) {
        if (m_options.top->unit == Top::Unit::Perc) {
            float mul = static_cast<float>(std::min(m_options.top->value, (unsigned)100)) / 100;
            rows.resize(rows.size() * mul, rows.back());
        }
        else {
            rows.resize(std::min(m_options.top->value, (unsigned)rows.size()), rows.back());
        }
    }

    std::vector<std::string> column_names;
    for (auto const& column : columns.columns()) {
        if (column.alias)
            column_names.push_back(*column.alias);
        else
            column_names.push_back(column.column->to_string());
    }

    std::vector<Tuple> output_rows;
    for (auto const& row : rows)
        output_rows.push_back(std::move(row.tuple));

    ResultSet result { column_names, std::move(output_rows) };

    if (context.db && m_options.select_into) {
        // TODO: Insert, not overwrite records
        if (context.db->exists(*m_options.select_into))
            TRY(context.db->drop_table(*m_options.select_into));
        TRY(context.db->create_table_from_query(std::move(result), *m_options.select_into));
    }
    return result;
}

DbErrorOr<std::vector<TupleWithSource>> Select::collect_rows(EvaluationContext& context, AbstractTable& table) const {
    auto& frame = context.current_frame();

    auto should_include_row = [&](Tuple const& row) -> DbErrorOr<bool> {
        if (!m_options.where)
            return true;
        frame.row = { .tuple = row, .source = {} };
        return TRY(m_options.where->evaluate(context)).to_bool();
    };

    // Collect all rows that should be included (applying WHERE and GROUP BY)
    // There rows are not yet SELECT'ed - they contain columns from table, no aliases etc.
    std::map<Tuple, std::vector<Tuple>> nonaggregated_row_groups;

    TRY(table.rows().try_for_each_row([&](Tuple const& row) -> DbErrorOr<void> {
        // WHERE
        if (!TRY(should_include_row(row)))
            return {};

        std::vector<Value> group_key;

        if (m_options.group_by) {
            for (const auto& column_name : m_options.group_by->columns) {
                // TODO: Handle aliases, indexes ("GROUP BY 1") and aggregate functions ("GROUP BY COUNT(x)")
                // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql?view=sql-server-ver16#g-using-group-by-with-an-expression
                auto column = table.get_column(column_name);
                if (!column) {
                    // TODO: Store source location info
                    if (m_options.group_by->type == GroupBy::GroupOrPartition::GROUP)
                        return DbError { "Nonexistent column used in GROUP BY: '" + column_name + "'", m_start };
                    else if (m_options.group_by->type == GroupBy::GroupOrPartition::PARTITION)
                        return DbError { "Nonexistent column used in PARTITION BY: '" + column_name + "'", m_start };
                }
                group_key.push_back(row.value(column->index));
            }
        }

        nonaggregated_row_groups[{ group_key }].push_back(row);
        return {};
    }));

    // Check if grouping / aggregation should be performed
    bool should_group = false;
    if (m_options.group_by) {
        should_group = true;
    }
    else {
        for (auto const& column : frame.columns.columns()) {
            if (column.column->contains_aggregate_function()) {
                should_group = true;
                break;
            }
        }
    }

    if (m_options.group_by->type == GroupBy::GroupOrPartition::PARTITION)
        should_group = false;

    // Special-case for empty sets
    if (table.size() == 0) {
        if (should_group) {
            // We need to create at least one group to make aggregate
            // functions return one row with value "0".
            nonaggregated_row_groups.insert({ Tuple {}, {} });
        }

        // Let's also check column expressions for validity, even
        // if they won't run on real rows.
        std::vector<Value> values;
        for (size_t s = 0; s < table.columns().size(); s++) {
            values.push_back(Value::null());
        }
        Tuple dummy_row { values };
        frame.row_group = std::span { &dummy_row, 1 };
        for (auto const& column : m_options.columns.columns()) {
            frame.row = { .tuple = dummy_row, .source = {} };
            TRY(column.column->evaluate(context));
        }
    }

    // std::cout << "should_group: " << should_group << std::endl;
    // std::cout << "nonaggregated_row_groups.size(): " << nonaggregated_row_groups.size() << std::endl;

    // Group + aggregate rows if needed, otherwise just evaluate column expressions
    std::vector<TupleWithSource> aggregated_rows;
    if (should_group) {
        auto should_include_group = [&](EvaluationContext& context, TupleWithSource const& row) -> DbErrorOr<bool> {
            if (!m_options.having)
                return true;
            frame.row = row;
            return TRY(m_options.having->evaluate(context)).to_bool();
        };

        auto is_in_group_by = [&](SelectColumns::Column const& column) {
            if (!m_options.group_by)
                return false;
            for (auto const& group_by_column : m_options.group_by->columns) {
                auto referenced_columns = column.column->referenced_columns();
                if (std::find(referenced_columns.begin(), referenced_columns.end(), group_by_column) != referenced_columns.end())
                    return true;
            }
            return false;
        };

        for (auto const& group : nonaggregated_row_groups) {
            frame.row_type = EvaluationContextFrame::RowType::FromTable;
            frame.row_group = group.second;
            std::vector<Value> values;
            for (auto& column : frame.columns.columns()) {
                if (column.column->contains_aggregate_function()) {
                    frame.row = {};
                    values.push_back(TRY(column.column->evaluate(context)));
                }
                else if (is_in_group_by(column)) {
                    frame.row = { .tuple = group.second[0], .source = {} };
                    values.push_back(TRY(column.column->evaluate(context)));
                }
                else {
                    // "All columns must be either aggregate or occur in GROUP BY clause"
                    // TODO: Store location info
                    return DbError { "Column '" + column.column->to_string() + "' must be either aggregate or occur in GROUP BY clause", m_start };
                }
            }

            frame.row_type = EvaluationContextFrame::RowType::FromResultSet;

            TupleWithSource aggregated_row { .tuple = { values }, .source = {} };

            // HAVING
            if (!TRY(should_include_group(context, aggregated_row)))
                continue;

            aggregated_rows.push_back(std::move(aggregated_row));
        }
    }
    else {
        for (auto const& group : nonaggregated_row_groups) {
            for (auto const& row : group.second) {
                std::vector<Value> values;
                frame.row_group = group.second;
                for (auto& column : frame.columns.columns()) {
                    frame.row = { .tuple = row, .source = row };
                    values.push_back(TRY(column.column->evaluate(context)));
                }
                aggregated_rows.push_back(TupleWithSource { .tuple = { values }, .source = row });
            }
        }
    }

    return aggregated_rows;
}

DbErrorOr<Value> SelectExpression::evaluate(EvaluationContext& context) const {
    auto result_set = TRY(m_select.execute(context));
    if (!result_set.is_convertible_to_value()) {
        // TODO: Store location info
        return DbError { "Select expression must return a single row with a single value", 0 };
    }
    return result_set.as_value();
}

DbErrorOr<std::unique_ptr<Table>> SelectTableExpression::evaluate(EvaluationContext& context) const {
    auto result = TRY(m_select.execute(context));

    auto table = std::make_unique<MemoryBackedTable>(nullptr, "");

    size_t i = 0;
    for (const auto& name : result.column_names()) {
        Value::Type type = result.rows().empty() ? Value::Type::Null : result.rows().front().value(i).type();
        TRY(table->add_column(Column(name, type, 0, 0, 0)));
        i++;
    }

    for (const auto& row : result.rows()) {
        TRY(table->insert(row));
    }

    return table;
}

DbErrorOr<ValueOrResultSet> SelectStatement::execute(Database& db) const {
    EvaluationContext context { .db = &db };
    return TRY(m_select.execute(context));
}

DbErrorOr<ValueOrResultSet> Union::execute(Database& db) const {
    EvaluationContext context { .db = &db };
    auto lhs = TRY(m_lhs.execute(context));
    auto rhs = TRY(m_rhs.execute(context));

    if (lhs.column_names().size() != rhs.column_names().size())
        return DbError { "Queries with different column count", 0 };

    for (size_t i = 0; i < lhs.column_names().size(); i++) {
        if (lhs.column_names()[i] != rhs.column_names()[i])
            return DbError { "Queries with different column set", 0 };
    }

    std::vector<Tuple> rows;

    for (const auto& row : lhs.rows()) {
        rows.push_back(row);
    }

    for (const auto& row : rhs.rows()) {
        if (m_distinct) {
            bool distinct = true;
            for (const auto& to_compare : lhs.rows()) {
                if (TRY(row == to_compare)) {
                    distinct = false;
                    break;
                }
            }

            if (distinct)
                rows.push_back(row);
        }
        else {
            rows.push_back(row);
        }
    }

    return ResultSet { lhs.column_names(), std::move(rows) };
}

DbErrorOr<std::optional<size_t>> SelectTableExpression::resolve_identifier(Database* db, Identifier const& id) const {
    assert(m_select.from());
    return m_select.from()->resolve_identifier(db, id);
}

DbErrorOr<size_t> SelectTableExpression::column_count(Database* db) const {
    assert(m_select.from());
    return m_select.from()->column_count(db);
}

}
