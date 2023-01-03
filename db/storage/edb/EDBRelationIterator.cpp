#include "EDBRelationIterator.hpp"
#include "db/storage/edb/Definitions.hpp"
#include <EssaUtil/Config.hpp>
#include <EssaUtil/Stream/MemoryStream.hpp>

namespace Db::Storage::EDB {

std::optional<Core::Tuple> EDBRelationIteratorImpl::next() {
    return next_impl().release_value_but_fixme_should_propagate_errors();
}

Util::OsErrorOr<std::optional<Core::Tuple>> EDBRelationIteratorImpl::next_impl() {
    if (m_file.m_header.row_count == 0) {
        return std::optional<Core::Tuple> {};
    }
    if (m_row_ptr.block == 0) {
        return std::optional<Core::Tuple> {};
    }

    // fmt::print("Last row: {}:{}\n", m_last_row_ptr.block, m_last_row_ptr.offset);
    auto row = m_file.access<Table::RowSpec>(m_row_ptr, m_file.row_size() + sizeof(Table::RowSpec));
    // fmt::print("rowspec {} {} {:x}\n", row->is_used, row->next_row.offset, row->row[0]);
    m_row_ptr = row->next_row;

    Util::ReadableMemoryStream stream { { row->row, m_file.row_size() } };
    Util::BinaryReader writer { stream };

    std::vector<Core::Value> values;
    for (auto const& column : m_file.m_columns) {
        auto is_null = column.not_null ? false : TRY(writer.read_little_endian<uint8_t>());
        switch (static_cast<Core::Value::Type>(column.type)) {
        case Core::Value::Type::Null:
            ESSA_UNREACHABLE;
            break;
        case Core::Value::Type::Int: {
            auto i = TRY(writer.read_little_endian<uint32_t>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_int(i));
            break;
        }
        case Core::Value::Type::Float: {
            auto f = TRY(writer.read_little_endian<float>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_float(f));
            break;
        }
        case Core::Value::Type::Varchar: {
            auto span = TRY(writer.read_struct<HeapSpan>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_varchar(m_file.read_heap(span).decode().encode()));
            break;
        }
        case Core::Value::Type::Bool: {
            auto b = TRY(writer.read_little_endian<uint8_t>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_bool(b));
            break;
        }
        case Core::Value::Type::Time: {
            auto time = TRY(writer.read_struct<Date>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_time(Core::Date { .year = time.year, .month = time.month, .day = time.day }));
            break;
        }
        }
    }

    // fmt::print("D: ");
    // for (auto const& v : values) {
    //     fmt::print("{} ", v.to_debug_string());
    // }
    // fmt::print("\n");

    return Core::Tuple { values };
}

}
