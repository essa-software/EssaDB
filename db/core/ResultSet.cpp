#include "ResultSet.hpp"

#include "Database.hpp"
#include "Table.hpp"
#include "Tuple.hpp"
#include <EssaUtil/UString.hpp>
#include <EssaUtil/Utf8.hpp>
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

void ResultSet::dump(std::ostream& out, FancyDump fancy) const {

    if (m_rows.empty()) {
        out << "Empty result set" << std::endl;
        return;
    }

    std::vector<int> widths;
    unsigned long index = 0;

    for (auto const& column : column_names()) {
        unsigned long max_width = column.size();

        for (auto const& row : m_rows) {
            Util::UString value { row.value(index).to_string().release_value_but_fixme_should_propagate_errors() };
            max_width = std::max(max_width, value.size());
        }
        index++;

        if (fancy == FancyDump::Yes) {
            out << "│ \e[1m" << std::setw(max_width) << std::left << column << " \e[m";
        }
        else {
            // FIXME: Port tests to left alignment
            out << "| " << std::setw(max_width) << std::right << column << " ";
        }
        widths.push_back(max_width);
    }
    out << (fancy == FancyDump::Yes ? "│" : "|") << "\n";

    if (fancy == FancyDump::Yes) {
        for (size_t s = 0; s < widths.size(); s++) {
            if (s == 0) {
                out << "├";
            }
            else if (s < widths.size()) {
                out << "┼";
            }
            for (int i = 0; i < widths[s] + 2; i++) {
                out << "─";
            }
            if (s == widths.size() - 1) {
                out << "┤";
            }
        }
        out << "\n";
    }

    for (auto const& row : m_rows) {
        index = 0;
        for (auto const& value : row) {
            out << (fancy == FancyDump::Yes ? "│ " : "| ");
            auto value_string = value.to_string().release_value_but_fixme_should_propagate_errors();;
            auto value_string_length = *Util::Utf8::codepoint_count_if_valid(value_string);
            int width = index < widths.size() ? widths[index] : 0;

            // FIXME: Port tests to left alignment
            if (fancy == FancyDump::No) {
                for (size_t s = 0; s < width - value_string_length; s++) {
                    out << " ";
                }
            }

            if (fancy == FancyDump::Yes && value.is_null()) {
                out << "\e[30mnull\e[m";
            }
            else {
                out << value_string;
            }

            // FIXME: Port tests to left alignment
            if (fancy == FancyDump::Yes) {
                for (size_t s = 0; s < width - value_string_length; s++) {
                    out << " ";
                }
            }
            out << " ";
            index++;
        }
        out << (fancy == FancyDump::Yes ? "│" : "|") << std::endl;
    }
}

}
