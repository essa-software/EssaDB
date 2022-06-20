#include "Table.hpp"
#include "Column.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Db::Core {

DbErrorOr<Table> Table::create_from_select_result(ResultSet const& select) {
    Table table;

    auto const& columns = select.column_names();
    auto const& rows = select.rows();
    size_t i = 0;

    for (const auto& col : columns) {
        TRY(table.add_column(Column(col, rows[0].value(i).type(), Column::AutoIncrement::No)));
    }

    table.m_rows = rows;
    return table;
}

DbErrorOr<void> Table::add_column(Column column) {
    if (get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Duplicate column '" + column.name() + "'", 0 };
    }
    m_columns.push_back(std::move(column));

    for (auto& row : m_rows) {
        row.extend();
    }

    return {};
}

DbErrorOr<void> Table::alter_column(Column column) {
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

DbErrorOr<void> Table::drop_column(Column column) {
    if (!get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Couldn't find column '" + column.name() + "'", 0 };
    }

    std::vector<Column> vec;

    for (size_t i = 0; i < m_columns.size(); i++) {
        if (m_columns[i].name() == column.name()) {
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

DbErrorOr<void> Table::update_cell(size_t row, size_t column, Value value) {
    m_rows[row].set_value(column, value);

    return {};
}

void Table::delete_row(size_t index) {
    std::vector<Tuple> vec;
    for (size_t i = 0; i < m_rows.size(); i++) {
        if (i != index)
            vec.push_back(m_rows[i]);
    }
    m_rows = std::move(vec);
}

DbErrorOr<void> Table::insert(RowWithColumnNames::MapType map) {
    m_rows.push_back(TRY(RowWithColumnNames::from_map(*this, map)).row());
    return {};
}

std::optional<std::pair<Column, size_t>> Table::get_column(std::string const& name) const {
    size_t index = 0;
    for (auto& column : m_columns) {
        if (column.name() == name)
            return { { column, index } };
        index++;
    }
    return {};
}

void Table::export_to_csv(const std::string& path) const {
    std::ofstream f_out(path);

    unsigned i = 0;

    for (const auto& col : m_columns) {
        f_out << col.name();

        if (i < m_columns.size() - 1)
            f_out << ',';
        else
            f_out << '\n';
        i++;
    }

    for (const auto& row : m_rows) {
        i = 0;
        for (auto it = row.begin(); it != row.end(); it++) {
            f_out << it->to_string().release_value();

            if (i < m_columns.size() - 1)
                f_out << ',';
            else
                f_out << '\n';
            i++;
        }
    }

    f_out.close();
}

DbErrorOr<void> Table::import_from_csv(const std::string& path) {
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
            bool quotes = false;
            if (c == '\'') {
                quotes = true;
                line_in.get();
                c = line_in.peek();
            }
            while (!((!quotes && c == ',') || (quotes && c == '\''))) {
                if (c == EOF)
                    break;
                value += c;
                line_in.get();
                c = line_in.peek();
            }
            if (line_in.peek() == '\'')
                line_in.get();
            line_in >> std::ws;
            if (line_in.peek() == ',')
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

    if (m_columns.empty()) {
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

            TRY(add_column(Column(column_name, type, Column::AutoIncrement::No)));

            column_index++;
        }
    }
    else {
        // std::cout << "Reading CSV into existing table" << std::endl;
        if (column_names.size() != m_columns.size())
            return DbError { "Column count differs in CSV file and in table", 0 };
    }

    for (auto const& row : rows) {
        RowWithColumnNames::MapType map;
        unsigned i = 0;

        for (const auto& col : m_columns) {
            auto value = row[i];
            i++;

            if (value == "null")
                continue;

            auto created_value = TRY(Value::from_string(col.type(), value));
            // std::cout << created_value.to_debug_string() << std::endl;
            map.insert({ col.name(), created_value });
        }

        TRY(insert(map));
    }

    return {};
}

}
