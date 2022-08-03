#include "Table.hpp"

#include "AbstractTable.hpp"
#include "Column.hpp"
#include "ResultSet.hpp"
#include "db/core/DbError.hpp"
#include "db/core/Expression.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Db::Core {

DbErrorOr<std::unique_ptr<MemoryBackedTable>> MemoryBackedTable::create_from_select_result(ResultSet const& select) {
    std::unique_ptr<MemoryBackedTable> table = std::make_unique<MemoryBackedTable>(nullptr);

    auto const& columns = select.column_names();
    auto const& rows = select.rows();
    size_t i = 0;

    for (const auto& col : columns) {
        TRY(table->add_column(Column(col, rows[0].value(i).type(), 0, 0, 0)));
    }
    table->m_rows = rows;
    return table;
}

DbErrorOr<void> MemoryBackedTable::add_column(Column column) {
    if (get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Duplicate column '" + column.name() + "'", 0 };
    }
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

DbErrorOr<void> MemoryBackedTable::insert(RowWithColumnNames::MapType map) {
    m_rows.push_back(TRY(RowWithColumnNames::from_map(*this, map)).row());
    return {};
}

DbErrorOr<void> MemoryBackedTable::insert(Tuple const& row) {
    m_rows.push_back(row);
    return {};
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
        RowWithColumnNames::MapType map;
        unsigned i = 0;

        for (const auto& col : columns) {
            auto value = row[i];
            i++;

            if (value == "null")
                continue;

            auto created_value = TRY(Value::from_string(col.type(), value));
            map.insert({ col.name(), created_value });
        }

        TRY(insert(map));
    }

    return {};
}

}
