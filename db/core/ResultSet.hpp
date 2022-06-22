#pragma once

#include "DbError.hpp"

#include <string>
#include <vector>

namespace Db::Core {

class Tuple;
class Value;

class ResultSet {
public:
    ResultSet(std::vector<std::string> column_names, std::vector<Tuple> rows);

    static ResultSet create_single_value(Value);

    // This must be out of line because of dependency cycle
    // See https://stackoverflow.com/questions/23984061/incomplete-type-for-stdvector
    ~ResultSet();

    std::vector<Tuple> const& rows() const { return m_rows; }
    std::vector<std::string> column_names() const { return m_column_names; }
    DbErrorOr<bool> compare(ResultSet const&) const;

    void dump(std::ostream& out) const;

    bool is_convertible_to_value() const;
    Value as_value() const;

private:
    std::vector<std::string> m_column_names;
    std::vector<Tuple> m_rows;
    std::vector<std::vector<Tuple>> m_buckets;
};

}
