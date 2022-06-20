#include "ResultSet.hpp"

#include "Database.hpp"
#include "Table.hpp"
#include "Tuple.hpp"

#include <iomanip>
#include <iostream>

namespace Db::Core {

ResultSet::ResultSet(std::vector<std::string> column_names, std::vector<Tuple> rows)
    : m_column_names(std::move(column_names))
    , m_rows(std::move(rows)) {
}

// This must be out of line because of dependency cycle
// See https://stackoverflow.com/questions/23984061/incomplete-type-for-stdvector
ResultSet::~ResultSet() = default;

void ResultSet::dump(std::ostream& out) const {
    std::vector<int> widths;
    unsigned long index = 0;

    for (auto const& column : column_names()) {
        unsigned long max_width = column.size();

        for (auto const& row : m_rows) {
            max_width = std::max(max_width, row.value(index).to_string().value().size());
        }
        index++;

        out << "| " << std::setw(max_width) << column << " ";
        widths.push_back(max_width);
    }
    out << "|" << std::endl;

    for (auto const& row : m_rows) {
        index = 0;
        for (auto const& value : row) {
            out << "| " << std::setw(index < widths.size() ? widths[index] : 0) << value;
            out << " ";
            index++;
        }
        out << "|" << std::endl;
    }
}

}
