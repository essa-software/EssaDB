#include "Table.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace Db::Core {

Table::Table(SelectResult const& select){
    auto columns = select.column_names();
    auto rows = select.rows();
    size_t i = 0;
    
    for(const auto& col : columns){
        auto column = add_column(Column(col, rows[0].value(i).type()));
    }

    for(const auto& row : rows){
        RowWithColumnNames::MapType map;

        for(size_t i = 0; i < columns.size(); i++){
            map.insert({columns[i], row.value(i)});
        }

        auto insert_result = insert(map);
    }
}

DbErrorOr<void> Table::add_column(Column column) {
    if (get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Duplicate column '" + column.name() + "'", 0 };
    }
    m_columns.push_back(std::move(column));

    for(auto& row : m_rows){
        row.extend();
    }
    
    return {};
}

DbErrorOr<void> Table::alter_column(Column column) {
    if (!get_column(column.name())) {
        // TODO: Save location info
        return DbError { "Couldn't find column '" + column.name() + "'", 0 };
    }

    for(size_t i = 0; i < m_columns.size(); i++){
        if(m_columns[i].name() == column.name()){
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

    for(size_t i = 0; i < m_columns.size(); i++){
        if(m_columns[i].name() == column.name()){
            for(auto& row : m_rows){
                row.remove(i);
            }
        }else {
            vec.push_back(std::move(m_columns[i]));
        }
    }

    m_columns = std::move(vec);

    return {};
}

void Table::delete_row(size_t index){
    std::vector<Tuple> vec;
    for(size_t i = 0; i < m_rows.size(); i++){
        if(i != index)
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
            default:
                break;
            }
            i++;
        }

        TRY(insert(map));
    }

    return {};
}

}
