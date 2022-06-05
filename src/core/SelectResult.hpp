#pragma once

#include "RowWithColumnNames.hpp"

namespace Db::Core {

class SelectResult {
public:
    SelectResult(std::vector<std::string> column_names, std::vector<Row> rows)
        : m_column_names(std::move(column_names))
        , m_rows(std::move(rows)) { }

    auto begin() const { return m_rows.begin(); }
    auto end() const { return m_rows.end(); }

    std::vector<std::string> column_names() const { return m_column_names; }

    void display() const;

private:
    std::vector<std::string> m_column_names;
    std::vector<Row> m_rows;
};

}
