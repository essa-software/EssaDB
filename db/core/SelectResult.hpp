#pragma once

#include <string>
#include <vector>

namespace Db::Core {

class Row;

// FIXME: This is somehow named in sql...
class SelectResult {
public:
    SelectResult(std::vector<std::string> column_names, std::vector<Row> rows);

    // This must be out of line because of dependency cycle
    // See https://stackoverflow.com/questions/23984061/incomplete-type-for-stdvector
    ~SelectResult();

    std::vector<Row> const& rows() const { return m_rows; }
    std::vector<std::string> column_names() const { return m_column_names; }

    void dump(std::ostream& out) const;

private:
    std::vector<std::string> m_column_names;
    std::vector<Row> m_rows;
};

}
