#include "AbstractTable.hpp"

namespace Db::Core {

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
