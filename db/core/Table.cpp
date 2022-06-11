#include "Table.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Db::Core {

Value::Type find_type(const std::string& str) {
    if (str == "null")
        return Value::Type::Null;

    for (const auto& c : str) {
        if (c < '0' || c > '9')
            return Value::Type::Varchar;
    }

    return Value::Type::Int;
}

DbErrorOr<void> Table::add_column(Column column) {
    if (get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Duplicate column '" + column.name() + "'", 0 };
    }
    m_columns.push_back(std::move(column));
    return {};
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
    m_rows.clear();
    m_columns.clear();

    std::ifstream f_in(path);

    char c = '\0';
    std::string str = "";
    std::vector<std::vector<std::string>> table(1);

    while (!f_in.eof()) {
        f_in.read(&c, 1);

        if (c == ',') {
            table.back().push_back(str);

            str = "";
        }
        else if (c == '\n') {
            table.back().push_back(str);
            table.push_back({});

            str = "";
        }
        else {
            str += c;
        }
    }
    table.pop_back();

    f_in.close();

    for (unsigned i = 0; i < table.begin()->size(); i++) {
        Value::Type type = Value::Type::Null;

        for (auto it = table.begin() + 1; it < table.end() - 1; it++) {

            if (type == Value::Type::Null)
                type = find_type((*it)[i]);
            else if (type == Value::Type::Int && find_type((*it)[i]) == Value::Type::Varchar)
                type = Value::Type::Varchar;
        }

        TRY(add_column(Column(table[0][i], type)));
    }

    for (auto it = table.begin() + 1; it < table.end() - 1; it++) {
        RowWithColumnNames::MapType map;
        unsigned i = 0;

        for (const auto& col : m_columns) {
            if ((*it)[i] == "null")
                continue;

            switch (col.type()) {
            case Value::Type::Int:
                map.insert({ col.name(), Value::create_int(std::stoi((*it)[i])) });
                break;
            case Value::Type::Varchar:
                map.insert({ col.name(), Value::create_varchar((*it)[i]) });
                break;
            case Value::Type::Bool:
                map.insert({ col.name(), Value::create_bool((*it)[i] == "true" ? true : false) });
                break;
            case Value::Type::Null:
            case Value::Type::SelectResult:
                break;
            }
            i++;
        }

        TRY(insert(map));
    }

    return {};
}

}
