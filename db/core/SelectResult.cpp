#include "SelectResult.hpp"

#include "Row.hpp"
#include "db/core/Database.hpp"
#include "db/core/Table.hpp"

#include <iomanip>
#include <iostream>

namespace Db::Core {

SelectResult::SelectResult(std::vector<std::string> column_names, std::vector<Row> rows)
    : m_column_names(std::move(column_names))
    , m_rows(std::move(rows)) {
}

// This must be out of line because of dependency cycle
// See https://stackoverflow.com/questions/23984061/incomplete-type-for-stdvector
SelectResult::~SelectResult() = default;

void SelectResult::dump(std::ostream& out) const {
    std::vector<int> widths;
    unsigned long index = 0;

    for (auto& column : column_names()) {
        unsigned long max_width = column.size();

        for (auto row : m_rows) {
            max_width = std::max(max_width, row.value(index).to_string().value().size());
        }
        index++;

        out << "| " << std::setw(max_width) << column << " ";
        widths.push_back(max_width);
    }
    out << "|" << std::endl;

    for (auto row : m_rows) {
        index = 0;
        for (auto value : row) {
            out << "| " << std::setw(widths[index]) << value;
            out << " ";
            index++;
        }
        out << "|" << std::endl;
    }
}

}
