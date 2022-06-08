#include "SelectResult.hpp"

#include "Row.hpp"

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
    for (auto& column : column_names()) {
        out << "| " << column << " ";
        widths.push_back(column.size());
    }
    out << "|" << std::endl;

    for (auto row : m_rows) {
        size_t index = 0;
        for (auto value : row) {
            out << "| " << std::setw(widths[index]) << value;
            out << " ";
            index++;
        }
        out << "|" << std::endl;
    }
}

}
