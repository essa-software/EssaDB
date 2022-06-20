#include "AbstractTable.hpp"

namespace Db::Core {

std::vector<Tuple> AbstractTableRowIterator::read_all() {
    std::vector<Tuple> result;
    for_each_row([&](Tuple const& row) {
        result.push_back(row);
    });
    return result;
}

std::optional<AbstractTable::ResolvedColumn> AbstractTable::get_column(std::string const& name) const {
    size_t index = 0;
    for (auto const& column : columns()) {
        if (column.name() == name)
            return { ResolvedColumn { .index = index, .column = column } };
        index++;
    }
    return {};
}

}
