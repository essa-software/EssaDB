#include "Relation.hpp"

namespace Db::Core {

std::optional<Tuple const*> Relation::find_first_matching_tuple(size_t column, Value value) {
    std::optional<Tuple const*> tuple;
    rows().for_each_row([&](auto const& row) {
        // FIXME: Stop iterating if value is found.
        if (tuple) {
            return;
        }
        if (row.value(column).type() != value.type()) {
            return;
        }
        if (MUST(row.value(column) == value)) {
            tuple = &row;
        }
    });
    return tuple;
}

std::optional<Relation::ResolvedColumn> Relation::get_column(std::string const& name) const {
    size_t index = 0;
    for (auto const& column : columns()) {
        if (column.name() == name)
            return { ResolvedColumn { .index = index, .column = column } };
        index++;
    }
    return {};
}

std::optional<Relation::ResolvedColumn> Relation::get_column(std::string const& name, Table* table) const {
    size_t index = 0;
    for (auto const& column : columns()) {
        if (column.name() == name && column.original_table() == table)
            return { ResolvedColumn { .index = index, .column = column } };
        index++;
    }
    return {};
}

}
