#include "Serializer.hpp"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/Error.hpp>
#include <cstdint>

#include <db/core/Table.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <db/storage/edb/EDBFile.hpp>

namespace Db::Storage::EDB {

Util::OsErrorOr<void> Serializer::write_column(EDBFile& file, Util::Writer& writer, Core::Column const& column) {
    TRY(writer.write_struct<Column>(Column {
        .column_name = TRY(file.copy_to_heap(column.name())),
        .type = static_cast<uint8_t>(column.type()),
        .auto_increment = column.auto_increment(),
        .unique = column.unique(),
        .not_null = column.not_null(),
        .default_value = { .varchar_value = {} },
    }));
    return {};
}

Util::OsErrorOr<void> Serializer::write_row(EDBFile& file, Util::Writer& writer, std::vector<Column> const& columns, Core::Tuple const& tuple) {
    assert(columns.size() == tuple.value_count());
    for (size_t s = 0; s < columns.size(); s++) {
        auto value = tuple.value(s);
        if (!columns[s].not_null) {
            TRY(writer.write_little_endian<uint8_t>(value.is_null()));
        }
        switch (static_cast<Core::Value::Type>(columns[s].type)) {
        case Core::Value::Type::Null:
            break;
        case Core::Value::Type::Int:
            TRY(writer.write_little_endian<uint32_t>(value.is_null() ? 0 : std::get<int>(value)));
            break;
        case Core::Value::Type::Float:
            TRY(writer.write_little_endian<float>(value.is_null() ? 0 : std::get<float>(value)));
            break;
        case Core::Value::Type::Varchar:
            TRY(writer.write_struct<HeapSpan>(TRY(file.copy_to_heap(std::get<std::string>(value)))));
            break;
        case Core::Value::Type::Bool:
            TRY(writer.write_little_endian<uint8_t>(value.is_null() ? false : std::get<bool>(value)));
            break;
        case Core::Value::Type::Time: {
            auto time = value.is_null() ? Core::Date {} : std::get<Core::Date>(value);
            TRY(writer.write_struct<Date>({ .year = time.year, .month = static_cast<uint8_t>(time.month), .day = static_cast<uint8_t>(time.day) }));
            break;
        }
        }
    }
    return {};
}

}
