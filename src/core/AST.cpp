#include "AST.hpp"

#include "Database.hpp"

namespace Db::Core::AST {

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

    std::vector<Row> rows;
    for (auto const& row : table->rows()) {
        // WHERE
        if (!TRY(should_include_row(row)))
            continue;

        // TODO: GROUP BY
        // TODO: HAVING

        // SELECT
        std::vector<Value> values;
        size_t index = 0;
        for (auto const& value : row) {
            if (m_columns.has(table->columns()[index].name()))
                values.push_back(value);
            index++;
        }
        rows.push_back(Row { values });
    }

    // TODO: DISTINCT
    // TODO: ORDER BY
    // TODO: TOP

    std::vector<std::string> column_names;
    for (auto const& column : table->columns()) {
        if (m_columns.has(column.name()))
            column_names.push_back(column.to_string());
    }

    return Value::create_select_result({ column_names, std::move(rows) });
}

}
