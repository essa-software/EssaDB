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

ResultSet ResultSet::create_single_value(Value value) {
    return ResultSet { { "tmp" }, { { std::move(value) } } };
}

// This must be out of line because of dependency cycle
// See https://stackoverflow.com/questions/23984061/incomplete-type-for-stdvector
ResultSet::~ResultSet() = default;

DbErrorOr<bool> ResultSet::compare(ResultSet const& other) const {
    if (m_rows.size() != other.m_rows.size())
        return false;
    for (size_t s = 0; s < m_rows.size(); s++) {
        auto compare = m_rows[s] == other.m_rows[s];
        if (compare.is_error())
            return false;
        if (!compare.release_value())
            return true;
    }
    return true;
}

bool ResultSet::is_convertible_to_value() const {
    return m_rows.size() == 1 && m_column_names.size() == 1;
}

Value ResultSet::as_value() const {
    return m_rows[0].value(0);
}

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
