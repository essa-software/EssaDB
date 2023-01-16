#include "Definitions.hpp"

#include <EssaUtil/ScopeGuard.hpp>
#include <db/storage/edb/EDBFile.hpp>

namespace Db::Storage::EDB {

uint8_t value_size_for_type(Core::Value::Type type) {
    switch (type) {
    case Core::Value::Type::Null:
        return 0;
    case Core::Value::Type::Int:
        return sizeof(Value::int_value);
    case Core::Value::Type::Float:
        return sizeof(Value::float_value);
    case Core::Value::Type::Varchar:
        return sizeof(Value::varchar_value);
    case Core::Value::Type::Bool:
        return sizeof(Value::bool_value);
    case Core::Value::Type::Time:
        return sizeof(Value::time_value);
    }
    ESSA_UNREACHABLE;
}

Util::OsErrorOr<void> Table::RowSpec::free_data(EDBFile& file) {
    size_t offset = 0;
    for (auto const& column : file.raw_columns()) {
        auto value_size = value_size_for_type(static_cast<Core::Value::Type>(column.type));
        Util::ScopeGuard guard {
            [&]() { offset += value_size; }
        };
        if (!column.not_null) {
            uint8_t is_null = row[offset];
            offset++;
            if (is_null) {
                continue;
            }
        }
        assert(offset + value_size <= file.row_size());
        AllocatingAlignedAccess<Value> value { &row[offset], value_size };

        switch (static_cast<Core::Value::Type>(column.type)) {
        case Core::Value::Type::Null:
        case Core::Value::Type::Int:
        case Core::Value::Type::Float:
        case Core::Value::Type::Bool:
        case Core::Value::Type::Time:
            break;
        case Core::Value::Type::Varchar:
            TRY(file.heap_free(value->varchar_value.offset));
        }
    }
    return {};
}

}
