#pragma once

#include <EssaUtil/Error.hpp>
#include <cstddef>
#include <cstdint>
#include <db/core/Value.hpp>
#include <db/storage/edb/Endian.hpp>
#include <sys/types.h>

namespace Db::Storage::EDB {

using BlockIndex = uint32_t;

class EDBFile;

constexpr uint8_t Magic[] = { 0x65, 0x73, 0x64, 0x62, 0x0d, 0x0a }; // esdb\r\n
constexpr uint16_t CurrentVersion = 0x0001;
constexpr size_t RowsPerBlock = 256;

struct [[gnu::packed]] HeapPtr {
    LittleEndian<BlockIndex> block;
    LittleEndian<uint32_t> offset;

    bool is_null() const { return block == 0; }
};

struct [[gnu::packed]] HeapSpan {
    HeapPtr offset;
    uint64_t size;
};

struct [[gnu::packed]] EDBHeader {
    uint8_t magic[6];
    LittleEndian<uint16_t> version;
    LittleEndian<uint32_t> block_size;
    LittleEndian<uint64_t> row_count;
    uint8_t column_count;
    HeapPtr first_row_ptr;
    HeapPtr last_row_ptr;
    BlockIndex last_table_block;
    BlockIndex last_heap_block;
    HeapSpan table_name;
    HeapSpan check_statement;
    uint8_t auto_increment_value_count;
    uint8_t key_count;
};

enum class BlockType : uint8_t {
    Free,
    Table,
    Heap,
    Big
};

struct [[gnu::packed]] Block {
    BlockType type;
    LittleEndian<BlockIndex> prev_block;
    LittleEndian<BlockIndex> next_block;
    uint8_t data[0];
};

struct Date {
    LittleEndian<uint16_t> year;
    uint8_t month;
    uint8_t day;
};

uint8_t value_size_for_type(Core::Value::Type type);

union Value {
    LittleEndian<uint32_t> int_value;
    LittleEndian<float> float_value;
    HeapSpan varchar_value;
    uint8_t bool_value;
    Date time_value;
};

static_assert(sizeof(Value) == 16);

struct Column {
    HeapSpan column_name;
    uint8_t type;
    uint8_t auto_increment;
    uint8_t unique;
    uint8_t not_null;
    uint8_t has_default_value;
    Value default_value;
};

namespace Table {

struct RowSpec {
    HeapPtr next_row;
    uint8_t is_used;
    uint8_t row[0];

    // Free heap-stored data on row's fields. This is only varchar string
    // for now. This doesn't mark row as unused.
    Util::OsErrorOr<void> free_data(EDBFile&);
};

static_assert(sizeof(RowSpec) == 9);

struct TableBlock {
    uint8_t rows_in_block;
    RowSpec rows[0];
};

}

}

template<>
class fmt::formatter<Db::Storage::EDB::HeapPtr> : public fmt::formatter<std::string_view> {
public:
    auto format(Db::Storage::EDB::HeapPtr const& p, fmt::format_context& ctx) const {
        fmt::format_to(ctx.out(), "{}:{}", p.block, p.offset);
        return ctx.out();
    }
};
