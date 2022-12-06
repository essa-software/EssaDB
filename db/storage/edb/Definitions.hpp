#pragma once

#include <cstddef>
#include <cstdint>
#include <db/storage/edb/Endian.hpp>
#include <sys/types.h>

namespace Db::Storage::EDB {

using BlockIndex = uint32_t;

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
    HeapPtr last_row_ptr;
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
    BlockType block_type;
    LittleEndian<BlockIndex> next_block;
    uint8_t data[0];
};

struct Date {
    LittleEndian<uint16_t> year;
    uint8_t month;
    uint8_t day;
};

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
    Value default_value;
};

namespace Table {

struct RowSpec {
    HeapPtr next_row;
    uint8_t is_used;
    uint8_t row[0];
};
static_assert(sizeof(RowSpec) == 9);

struct TableBlock {
    uint8_t rows_in_block;
    RowSpec rows[0];
};

}

}
