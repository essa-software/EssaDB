#include "CSVFile.hpp"

#include <db/core/TupleFromValues.hpp>
#include <db/core/Value.hpp>
#include <fstream>

namespace Db::Storage {

Core::DbErrorOr<CSVFile> CSVFile::import(std::string const& path, std::vector<Core::Column> const& column_hint) {
    CSVFile file;
    TRY(file.do_import(path, column_hint));
    return file;
}

Core::DbErrorOr<void> CSVFile::do_import(std::string const& path, std::vector<Core::Column> const& column_hint) {
    std::ifstream f_in(path);
    f_in >> std::ws;
    if (!f_in.good())
        return Core::DbError { "Failed to open CSV file '" + path + "': " + std::string(strerror(errno)) };

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

    std::vector<std::string> column_names;
    std::vector<std::vector<std::string>> rows;

    column_names = read_line();
    if (column_names.empty())
        return Core::DbError { "CSV file contains no columns" };

    while (true) {
        auto row_line = read_line();
        if (row_line.empty())
            break;
        if (row_line.size() != column_names.size()) {
            return Core::DbError { "Invalid value count in row, expected " + std::to_string(column_names.size()) + ", got " + std::to_string(row_line.size()) };
        }
        rows.push_back(std::move(row_line));
    }

    f_in.close();

    if (column_hint.empty()) {
        size_t column_index = 0;
        for (auto const& column_name : column_names) {
            Core::Value::Type type = Core::Value::Type::Null;

            for (auto const& row : rows) {
                if (type == Core::Value::Type::Null) {
                    auto new_type = Core::find_type(row[column_index]);
                    if (new_type != Core::Value::Type::Null)
                        type = new_type;
                }
                else if (type == Core::Value::Type::Int && Core::find_type(row[column_index]) == Core::Value::Type::Varchar)
                    type = Core::Value::Type::Varchar;
            }

            m_columns.push_back(Core::Column(column_name, type, false, false, false));
            column_index++;
        }
    }
    else {
        m_columns = column_hint;
    }

    for (auto const& row : rows) {
        std::vector<Core::Value> tuple;
        unsigned i = 0;

        for (const auto& col : m_columns) {
            auto const& value = row[i];
            i++;
            if (value == "null") {
                tuple.push_back(Core::Value::null());
            }
            else {
                auto created_value = TRY(Core::Value::from_string(col.type(), value));
                tuple.push_back(std::move(created_value));
            }
        }

        m_rows.push_back(Core::Tuple { tuple });
    }

    return {};
}

}
