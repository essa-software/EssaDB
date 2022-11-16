#include "Serializer.hpp"

#include <EssaUtil/Error.hpp>
#include <db/core/Table.hpp>

namespace Db::Storage {

static constexpr uint8_t dummy_dataspan[14] {};
static constexpr uint8_t dummy_dynamic_value[15] {};

Util::OsErrorOr<void> Serializer::write_table(Core::Table const& table) {
    // Header
    TRY(m_writer.write_little_endian<uint64_t>(table.size()));
    TRY(m_writer.write_little_endian<uint8_t>(table.columns().size()));
    TRY(m_writer.write(dummy_dataspan));            // TODO: Table name
    TRY(m_writer.write(dummy_dataspan));            // TODO: Check SQL statement
    TRY(m_writer.write_little_endian<uint8_t>(0));  // TODO: Number of AI variables
    TRY(m_writer.write_little_endian<uint8_t>(0));  // TODO: Number of keys
    TRY(m_writer.write_little_endian<uint64_t>(0)); // TODO: Index of last known free row chunk
    for (auto const& column : table.columns()) {
        TRY(write_column(column));
    }
    TRY(table.rows().try_for_each_row([&](Core::Tuple const& row) -> Util::OsErrorOr<void> {
        TRY(write_row(row));
        return {};
    }));
    return {};
}

Util::OsErrorOr<void> Serializer::write_column(Core::Column const& column) {
    TRY(m_writer.write(dummy_dataspan)); // TODO: Column name
    TRY(m_writer.write_little_endian(static_cast<uint8_t>(column.type())));
    TRY(m_writer.write_little_endian<uint8_t>(column.auto_increment())); // AI
    TRY(m_writer.write_little_endian<uint8_t>(column.unique()));         // UN
    TRY(m_writer.write_little_endian<uint8_t>(column.not_null()));       // NN
    TRY(m_writer.write(dummy_dynamic_value));                            // TODO: Default value
    return {};
}

Util::OsErrorOr<void> Serializer::write_row(Core::Tuple const& tuple) {
    for (auto const& value : tuple) {
        TRY(write_value(value));
    }
    return {};
}

Util::OsErrorOr<void> Serializer::write_value(Core::Value const& value) {
    TRY(std::visit(
        Util::Overloaded {
            [&](std::monostate) -> Util::OsErrorOr<void> {
                return {};
            },
            [&](int value) -> Util::OsErrorOr<void> {
                TRY(m_writer.write_little_endian<int32_t>(value));
                return {};
            },
            [&](float value) -> Util::OsErrorOr<void> {
                static_assert(sizeof(float) == sizeof(uint32_t));
                TRY(m_writer.write_little_endian<uint32_t>(std::bit_cast<uint32_t>(value)));
                return {};
            },
            [&](std::string const&) -> Util::OsErrorOr<void> {
                // TODO
                return {};
            },
            [&](bool value) -> Util::OsErrorOr<void> {
                TRY(m_writer.write_little_endian<uint8_t>(value ? 1 : 0));
                return {};
            },
            [&](Core::Date date) -> Util::OsErrorOr<void> {
                TRY(m_writer.write_little_endian<uint16_t>(date.year));
                TRY(m_writer.write_little_endian<uint8_t>(date.month));
                TRY(m_writer.write_little_endian<uint8_t>(date.day));
                return {};
            },
        },
        value));
    return {};
}

}
