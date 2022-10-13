#include "Table.hpp"

#include <cstring>
#include <db/core/AbstractTable.hpp>
#include <db/core/Column.hpp>
#include <db/core/DbError.hpp>
#include <db/core/ResultSet.hpp>
#include <db/core/TupleFromValues.hpp>
#include <db/core/ast/Expression.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Db::Core {

DbErrorOr<std::unique_ptr<MemoryBackedTable>> MemoryBackedTable::create_from_select_result(ResultSet const& select) {
    std::unique_ptr<MemoryBackedTable> table = std::make_unique<MemoryBackedTable>(nullptr, "");

    auto const& columns = select.column_names();
    auto const& rows = select.rows();

    size_t i = 0;
    for (const auto& col : columns) {
        TRY(table->add_column(Column(col, rows[0].value(i).type(), false, false, false)));
        i++;
    }
    table->m_rows = rows;
    return table;
}

DbErrorOr<void> MemoryBackedTable::add_column(Column column) {
    if (!column.original_table())
        column.set_table(this);
    m_columns.push_back(std::move(column));

    for (auto& row : m_rows) {
        row.extend();
    }

    return {};
}

DbErrorOr<void> MemoryBackedTable::alter_column(Column column) {
    if (!get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Couldn't find column '" + column.name() + "'", 0 };
    }

    for (size_t i = 0; i < m_columns.size(); i++) {
        if (m_columns[i].name() == column.name()) {
            m_columns[i] = std::move(column);
        }
    }
    return {};
}

DbErrorOr<void> MemoryBackedTable::drop_column(std::string const& column) {
    if (!get_column(column)) {
        // TODO: Save location info
        return DbError { "Couldn't find column '" + column + "'", 0 };
    }

    std::vector<Column> vec;

    for (size_t i = 0; i < m_columns.size(); i++) {
        if (m_columns[i].name() == column) {
            for (auto& row : m_rows) {
                row.remove(i);
            }
        }
        else {
            vec.push_back(std::move(m_columns[i]));
        }
    }

    m_columns = std::move(vec);

    return {};
}

DbErrorOr<void> MemoryBackedTable::insert(Tuple const& row) {
    if (row.value_count() != m_columns.size()) {
        dump_structure();
        return DbError { fmt::format("Internal error: insert(): Column count does not match ({} given vs {} required)", row.value_count(), m_columns.size()), 0 };
    }
    for (size_t s = 0; s < m_columns.size(); s++) {
        if (m_columns[s].not_null() && row.value(s).is_null()) {
            dump_structure();
            return DbError { fmt::format("Internal error: insert(): Column {} is set to be null but is not NOT NULL", s), 0 };
        }
        if (!row.value(s).is_null() && m_columns[s].type() != row.value(s).type()) {
            dump_structure();
            return DbError { fmt::format("Internal error: insert(): Type mismatch, required {} but given {} at index {}", Value::type_to_string(m_columns[s].type()), Value::type_to_string(row.value(s).type()), s), 0 };
        }
        // assert(m_columns[s].type() == row.value(s).type() || (!m_columns[s].not_null() && row.value(s).is_null()));
    }
    m_rows.push_back(row);
    return {};
}

void MemoryBackedTable::dump_structure() const {
    fmt::print("MemoryBackedTable '{}' @{}\n", name(), fmt::ptr(this));
    fmt::print("Rows: {}\n", raw_rows().size());
    fmt::print("Columns:\n");
    for (auto const& c : m_columns) {
        fmt::print(" - {} {}\n", c.name(), Value::type_to_string(c.type()));
    }
}

void Table::export_to_csv(const std::string& path) const {
    std::ofstream f_out(path);

    unsigned i = 0;

    auto const& columns = this->columns();

    for (const auto& col : columns) {
        f_out << col.name();

        if (i < columns.size() - 1)
            f_out << ',';
        else
            f_out << '\n';
        i++;
    }

    rows().for_each_row([&](auto const& row) {
        i = 0;
        for (auto it = row.begin(); it != row.end(); it++) {
            f_out << it->to_string().release_value();

            if (i < columns.size() - 1)
                f_out << ',';
            else
                f_out << '\n';
            i++;
        }
    });
}

DbErrorOr<void> Table::import_from_csv(Database& db, const std::string& path) {
    std::ifstream f_in(path);
    f_in >> std::ws;
    if (!f_in.good())
        return DbError { "Failed to open CSV file '" + path + "': " + std::string(strerror(errno)), 0 };

    std::vector<std::string> column_names;
    std::vector<std::vector<std::string>> rows;

    auto read_line = [&]() -> std::vector<std::string> {
        std::string line;
        if (!std::getline(f_in, line))
            return {};

        std::vector<std::string> values;
        std::istringstream line_in { line };
        while (true) {
            line_in >> std::ws;
            std::string value;

            char c = line_in.peek();
            if (c == EOF)
                break;
            std::optional<char> quote;
            if (c == '\'' || c == '"') {
                quote = c;
                line_in.get();
                c = line_in.peek();
            }
            while (!((!quote && (c == ',' || c == ';')) || (quote && c == *quote))) {
                if (c == EOF)
                    break;
                value += c;
                line_in.get();
                c = line_in.peek();
            }
            if (line_in.peek() == '\'' || line_in.peek() == '"')
                line_in.get();
            line_in >> std::ws;
            if (line_in.peek() == ',' || line_in.peek() == ';')
                line_in.get();
            values.push_back(std::move(value));
        }
        return values;
    };

    column_names = read_line();
    if (column_names.empty())
        return DbError { "CSV file contains no columns", 0 };

    while (true) {
        auto row_line = read_line();
        if (row_line.empty())
            break;
        if (row_line.size() != column_names.size()) {
            return DbError { "Invalid value count in row, expected " + std::to_string(column_names.size()) + ", got " + std::to_string(row_line.size()), 0 };
        }
        rows.push_back(std::move(row_line));
    }

    f_in.close();

    if (columns().empty()) {
        size_t column_index = 0;
        for (auto const& column_name : column_names) {
            Value::Type type = Value::Type::Null;

            for (auto const& row : rows) {

                if (type == Value::Type::Null) {
                    auto new_type = find_type(row[column_index]);
                    if (new_type != Value::Type::Null)
                        type = new_type;
                }
                else if (type == Value::Type::Int && find_type(row[column_index]) == Value::Type::Varchar)
                    type = Value::Type::Varchar;
            }

            TRY(add_column(Column(column_name, type, 0, 0, 0)));

            column_index++;
        }
    }
    else {
        // std::cout << "Reading CSV into existing table" << std::endl;
        if (column_names.size() != columns().size())
            return DbError { "Column count differs in CSV file and in table", 0 };
    }

    auto columns = this->columns();
    for (auto const& row : rows) {
        std::vector<std::pair<std::string, Value>> tuple;
        unsigned i = 0;

        for (const auto& col : columns) {
            auto value = row[i];
            i++;

            if (value == "null")
                continue;

            auto created_value = TRY(Value::from_string(col.type(), value));
            tuple.push_back({ col.name(), std::move(created_value) });
        }

        TRY(insert(TRY(create_tuple_from_values(db, *this, tuple))));
    }

    return {};
}

}
