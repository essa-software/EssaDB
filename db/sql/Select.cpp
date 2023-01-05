#include <db/sql/Select.hpp>

#include <EssaUtil/Is.hpp>
#include <EssaUtil/ScopeGuard.hpp>
#include <cstddef>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>
#include <db/core/Table.hpp>
#include <db/core/Tuple.hpp>
#include <db/core/Value.hpp>
#include <db/sql/Printing.hpp>
#include <db/sql/SQLError.hpp>
#include <memory>

namespace Db::Sql::AST {

SQLErrorOr<Core::ResultSet> Select::execute(EvaluationContext& context) const {
    // Comments specify SQL Conceptional Evaluation:
    // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql#logical-processing-order-of-the-select-statement
    // FROM

    std::unique_ptr<Core::Relation> relation = m_options.from ? TRY(m_options.from->evaluate(context)) : nullptr;

    SelectColumns select_all_columns;

    SelectColumns const& columns = *TRY([this, table = relation.get(), &select_all_columns]() -> SQLErrorOr<SelectColumns const*> {
        if (m_options.columns.select_all()) {
            if (!table) {
                return SQLError { "You need a table to do SELECT *", m_start };
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

    auto rows = TRY([&]() -> SQLErrorOr<std::vector<Core::TupleWithSource>> {
        if (m_options.from) {
            // SELECT etc.
            // TODO: Make use of iterator capabilities of this instead of
            //       reading everything into memory.
            return collect_rows(context, *relation);
        }

        std::vector<Core::Value> values;
        for (auto const& column : m_options.columns.columns()) {
            values.push_back(TRY(column.column->evaluate(context)));
        }
        return std::vector<Core::TupleWithSource> { { .tuple = Core::Tuple { values }, .source = {} } };
    }());

    frame.row_type = EvaluationContextFrame::RowType::FromResultSet;

    // DISTINCT
    if (m_options.distinct) {
        std::vector<Core::TupleWithSource> occurences;

        // FIXME: O(n^2)
        for (const auto& row : rows) {
            bool distinct = true;
            for (const auto& to_compare : occurences) {
                if (TRY((row == to_compare).map_error(DbToSQLError { m_start }))) {
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
        auto generate_tuple_pair_for_ordering = [&](Core::TupleWithSource const& lhs, Core::TupleWithSource const& rhs) -> SQLErrorOr<std::pair<Core::Tuple, Core::Tuple>> {
            std::vector<Core::Value> lhs_values;
            std::vector<Core::Value> rhs_values;

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

            return std::pair<Core::Tuple, Core::Tuple> { Core::Tuple { lhs_values }, Core::Tuple { rhs_values } };
        };

        std::optional<SQLError> error;
        std::stable_sort(rows.begin(), rows.end(), [&](Core::TupleWithSource const& lhs, Core::TupleWithSource const& rhs) -> bool {
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
            rows.resize(rows.size() * mul, Core::TupleWithSource { {}, {} });
        }
        else {
            rows.resize(std::min<size_t>(m_options.top->value, rows.size()), Core::TupleWithSource { {}, {} });
        }
    }

    std::vector<std::string> column_names;
    for (auto const& column : columns.columns()) {
        if (column.alias)
            column_names.push_back(*column.alias);
        else
            column_names.push_back(column.column->to_string());
    }

    std::vector<Core::Tuple> output_rows;
    for (auto const& row : rows)
        output_rows.push_back(std::move(row.tuple));

    Core::ResultSet result { column_names, std::move(output_rows) };

    if (context.db && m_options.select_into) {
        // TODO: Insert, not overwrite records
        if (context.db->exists(*m_options.select_into))
            TRY(context.db->drop_table(*m_options.select_into).map_error(DbToSQLError { m_start }));
        TRY(context.db->create_table_from_query(std::move(result), *m_options.select_into).map_error(DbToSQLError { m_start }));
    }
    return result;
}

SQLErrorOr<std::vector<Core::TupleWithSource>> Select::collect_rows(EvaluationContext& context, Core::Relation& table) const {
    auto& frame = context.current_frame();

    auto should_include_row = [&](Core::Tuple const& row) -> SQLErrorOr<bool> {
        if (!m_options.where)
            return true;
        frame.row = { .tuple = row, .source = {} };
        return TRY(m_options.where->evaluate(context)).to_bool().map_error(DbToSQLError { m_start });
    };

    // Collect all rows that should be included (applying WHERE and GROUP BY)
    // There rows are not yet SELECT'ed - they contain columns from table, no aliases etc.
    std::map<Core::Tuple, std::vector<Core::Tuple>> nonaggregated_row_groups;

    TRY(table.rows().try_for_each_row([&](Core::Tuple const& row) -> SQLErrorOr<void> {
        // WHERE
        if (!TRY(should_include_row(row)))
            return {};

        std::vector<Core::Value> group_key;

        if (m_options.group_by) {
            for (const auto& column_name : m_options.group_by->columns) {
                // TODO: Handle aliases, indexes ("GROUP BY 1") and aggregate functions ("GROUP BY COUNT(x)")
                // https://docs.microsoft.com/en-us/sql/t-sql/queries/select-transact-sql?view=sql-server-ver16#g-using-group-by-with-an-expression
                auto column = table.get_column(column_name);
                if (!column) {
                    if (m_options.group_by->type == GroupBy::GroupOrPartition::GROUP)
                        return SQLError { "Nonexistent column used in GROUP BY: '" + column_name + "'", m_start };
                    else if (m_options.group_by->type == GroupBy::GroupOrPartition::PARTITION)
                        return SQLError { "Nonexistent column used in PARTITION BY: '" + column_name + "'", m_start };
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
            nonaggregated_row_groups.insert({ Core::Tuple {}, {} });
        }

        // Let's also check column expressions for validity, even
        // if they won't run on real rows.
        std::vector<Core::Value> values;
        for (size_t s = 0; s < table.columns().size(); s++) {
            values.push_back(Core::Value::null());
        }
        Core::Tuple dummy_row { values };
        frame.row_group = std::span { &dummy_row, 1 };
        for (auto const& column : m_options.columns.columns()) {
            frame.row = { .tuple = dummy_row, .source = {} };
            TRY(column.column->evaluate(context));
        }
    }

    // std::cout << "should_group: " << should_group << std::endl;
    // std::cout << "nonaggregated_row_groups.size(): " << nonaggregated_row_groups.size() << std::endl;

    // Group + aggregate rows if needed, otherwise just evaluate column expressions
    std::vector<Core::TupleWithSource> aggregated_rows;
    if (should_group) {
        auto should_include_group = [&](EvaluationContext& context, Core::TupleWithSource const& row) -> SQLErrorOr<bool> {
            if (!m_options.having)
                return true;
            frame.row = row;
            return TRY(m_options.having->evaluate(context)).to_bool().map_error(DbToSQLError { m_start });
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
            std::vector<Core::Value> values;
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
                    // TODO: Store (better) location info
                    return SQLError { "Column '" + column.column->to_string() + "' must be either aggregate or occur in GROUP BY clause", m_start };
                }
            }

            frame.row_type = EvaluationContextFrame::RowType::FromResultSet;

            Core::TupleWithSource aggregated_row { .tuple = { values }, .source = {} };

            // HAVING
            if (!TRY(should_include_group(context, aggregated_row)))
                continue;

            aggregated_rows.push_back(std::move(aggregated_row));
        }
    }
    else {
        for (auto const& group : nonaggregated_row_groups) {
            for (auto const& row : group.second) {
                std::vector<Core::Value> values;
                frame.row_group = group.second;
                for (auto& column : frame.columns.columns()) {
                    frame.row = { .tuple = row, .source = row };
                    values.push_back(TRY(column.column->evaluate(context)));
                }
                aggregated_rows.push_back(Core::TupleWithSource { .tuple = { values }, .source = row });
            }
        }
    }

    return aggregated_rows;
}

std::string Select::to_string() const {
    std::string string = "SELECT ";

    // DISTINCT
    if (m_options.distinct) {
        string += "DISTINCT ";
    }

    // TOP
    if (m_options.top) {
        string += "TOP ";
        string += std::to_string(m_options.top->value) + " ";
        if (m_options.top->unit == Top::Unit::Perc) {
            string += "PERC ";
        }
    }

    // Columns
    if (m_options.columns.select_all()) {
        string += "*";
    }
    else {
        auto const& columns = m_options.columns.columns();
        size_t s = 0;
        for (auto const& column : columns) {
            string += column.column->to_string();
            if (column.alias) {
                string += " AS " + Printing::escape_identifier(*column.alias);
            }
            if (s != columns.size() - 1) {
                string += ", ";
            }
            s++;
        }
    }

    // INTO
    if (m_options.select_into) {
        string += " INTO " + Printing::escape_identifier(*m_options.select_into);
    }

    // FROM
    if (m_options.from) {
        string += " FROM " + m_options.from->to_string();
    }

    // GROUP BY
    if (m_options.group_by) {
        string += m_options.group_by->type == GroupBy::GroupOrPartition::GROUP ? " GROUP BY " : " PARTITION BY ";
        for (size_t s = 0; s < m_options.group_by->columns.size(); s++) {
            string += Printing::escape_identifier(m_options.group_by->columns[s]);
            if (s != m_options.group_by->columns.size() - 1) {
                string += ", ";
            }
        }
    }

    // HAVING
    if (m_options.having) {
        string += " HAVING " + m_options.having->to_string();
    }

    // ORDER BY
    if (m_options.order_by) {
        string += " ORDER BY ";
        for (size_t s = 0; s < m_options.order_by->columns.size(); s++) {
            string += m_options.order_by->columns[s].expression->to_string() + " ";
            string += m_options.order_by->columns[s].order == OrderBy::Order::Ascending ? "ASC" : "DESC";
            if (s != m_options.order_by->columns.size() - 1) {
                string += ", ";
            }
        }
    }

    return string;
}

}
