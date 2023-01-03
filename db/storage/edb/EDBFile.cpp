#include "EDBFile.hpp"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/Error.hpp>
#include <EssaUtil/ScopeGuard.hpp>
#include <EssaUtil/Stream/File.hpp>
#include <EssaUtil/Stream/Stream.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <db/storage/edb/MappedFile.hpp>
#include <db/storage/edb/Serializer.hpp>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

namespace Db::Storage::EDB {

Util::OsErrorOr<void> ftruncate(int fd, off_t size) {
    if (::ftruncate(fd, size) < 0) {
        return Util::OsError { .error = errno, .function = "ftruncate" };
    }
    return {};
}

EDBFile::EDBFile(Util::File f, MappedFile mapped_file)
    : m_mapped_file(std::move(mapped_file))
    , m_file(std::move(f)) {
}

Util::OsErrorOr<std::unique_ptr<EDBFile>> EDBFile::open(std::string const& path) {
    Util::File file { ::open(path.c_str(), O_RDWR), true };
    struct stat stat;
    if (::fstat(file.fd(), &stat) < 0) {
        return Util::OsError { .error = errno, .function = "EDBFile::open(): stat" };
    }
    auto mapped_file = TRY(MappedFile::map(file.fd(), stat.st_size));
    auto edb_file = std::unique_ptr<EDBFile>(new EDBFile(std::move(file), std::move(mapped_file)));
    TRY(edb_file->read_header());
    return edb_file;
}

Util::OsErrorOr<std::unique_ptr<EDBFile>> EDBFile::initialize(std::string const& path, Db::Core::TableSetup setup) {
    Util::File file { ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644), true };
    if (file.fd() == -1) {
        return Util::OsError { .error = errno, .function = "EDBFile::initialize() open" };
    }
    auto mapped_file = TRY(MappedFile::map(file.fd(), sizeof(EDBHeader)));
    auto edb_file = std::unique_ptr<EDBFile>(new EDBFile(std::move(file), std::move(mapped_file)));

    TRY(edb_file->write_header_first_pass(setup));

    // Allocate blocks for table and data.
    TRY(edb_file->allocate_block(BlockType::Table));
    TRY(edb_file->allocate_block(BlockType::Heap));

    TRY(edb_file->write_header(setup));
    TRY(edb_file->read_header());

    return edb_file;
}

void EDBFile::dump_blocks() {
    fmt::print("Block count: {}\n", m_block_count);
    for (size_t s = 1; s < m_block_count; s++) {
        auto block = access<Block>({ s, 0 });
        auto type = block->type;
        fmt::print("- {}: prev={} next={} type=", s, block->prev_block, block->next_block);
        switch (type) {
        case BlockType::Free:
            fmt::print("FREE");
            break;
        case BlockType::Table:
            fmt::print("TABLE");
            break;
        case BlockType::Heap:
            fmt::print("HEAP");
            break;
        case BlockType::Big:
            fmt::print("BIG");
            break;
        }
        fmt::print("\n");
    }
}

Util::Buffer EDBFile::read_heap(HeapSpan span) const {
    auto ptr = heap_ptr_to_mapped_ptr(span.offset);
    // fmt::print("read_heap({}:{} +{}) = [", span.offset.block, span.offset.offset, (uint32_t)span.size);
    // for (auto b : std::span { ptr, span.size }) {
    //     fmt::print("{:02x} ", static_cast<uint16_t>(b));
    // }
    // fmt::print("]\n");
    return Util::Buffer { { ptr, span.size } };
}

size_t EDBFile::header_size() const {
    // TODO: AI, keys
    return sizeof(EDBHeader) + m_header.column_count * sizeof(Column);
}

size_t EDBFile::block_size() const {
    return m_header.block_size;
}

size_t EDBFile::block_offset(BlockIndex idx) const {
    // fmt::print("{} + {} * ({} - 1)\n", header_size(), block_size(), idx);
    return header_size() + block_size() * (idx - 1);
}

uint8_t* EDBFile::heap_ptr_to_mapped_ptr(HeapPtr ptr) {
    return reinterpret_cast<uint8_t*>(m_mapped_file.data().data() + block_offset(ptr.block) + ptr.offset);
}

uint8_t const* EDBFile::heap_ptr_to_mapped_ptr(HeapPtr ptr) const {
    return reinterpret_cast<uint8_t const*>(m_mapped_file.data().data() + block_offset(ptr.block) + ptr.offset);
}

Util::OsErrorOr<void> EDBFile::expand(size_t blocks) {
    TRY(ftruncate(m_file.fd(), m_file_size + blocks * block_size()));
    m_file_size += blocks * block_size();
    m_block_count += blocks;
    // fmt::print("Remap to size={} block_size={}\n", m_file_size, block_size());
    TRY(m_mapped_file.remap(m_file_size));
    return {};
}

Util::OsErrorOr<BlockIndex> EDBFile::allocate_block(BlockType block_type) {
    // fmt::print("!!!!! allocate block\n");

    BlockIndex allocated_block = 0;
    for (BlockIndex s = 1; s < m_block_count; s++) {
        // fmt::print("Checking block: {}\n", s);
        auto block = access<Block>({ s, 0 });
        if (block->type == BlockType::Free) {
            allocated_block = s;
            break;
        }
    }
    // fmt::print("!!! expand {}\n", m_block_count);
    TRY(expand(1));
    allocated_block = m_block_count - 1;

    auto block = access<Block>({ allocated_block, 0 }, block_size());
    block->type = block_type;

    switch (block_type) {
    case BlockType::Free:
        break;
    case BlockType::Table: {
        if (m_header.last_table_block != 0) {
            auto last_block = access<Block>({ m_header.last_table_block, 0 });
            last_block->next_block = allocated_block;
        }
        block->prev_block = m_header.last_table_block;
        m_header.last_table_block = allocated_block;
        break;
    }
    case BlockType::Heap: {
        if (m_header.last_heap_block != 0) {
            auto last_block = access<Block>({ m_header.last_heap_block, 0 });
            last_block->next_block = allocated_block;
        }
        block->prev_block = m_header.last_heap_block;
        m_header.last_heap_block = allocated_block;
        // This would override heap if not flushed here!
        block.flush();
        block.clear();
        // fmt::print("Initializing heap block @ {}\n", allocated_block);
        auto heap_block = access<Data::HeapBlock>({ allocated_block, sizeof(Block) }, block_size() - sizeof(Block));
        heap_block->init(*this);
        // fmt::print("heap before flushing is at {:x}\n", heap_block.original_ptr() - m_mapped_file.data().data());
        heap_block.flush();
        // fmt::print("Heap dump just after initializing:\n");
        // m_heap.dump();
        break;
    }
    case BlockType::Big:
        break;
    }

    block.flush();
    // dump_blocks();
    return allocated_block;
}

static uint8_t value_size_for_type(Core::Value::Type type) {
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

Util::OsErrorOr<void> EDBFile::write_header_first_pass(Db::Core::TableSetup const& setup) {
    uint32_t block_size = 0;
    block_size += sizeof(Table::RowSpec) - 1;
    for (auto const& column : setup.columns) {
        if (!column.not_null()) {
            block_size += 1;
        }
        block_size += value_size_for_type(column.type());
    }
    block_size *= 255;
    block_size += sizeof(Table::TableBlock);

    // This will be overridden later, but it is needed for allocate_block and heap_allocate to work
    m_header.block_size = block_size;
    // fmt::print("Block size: {}\n", block_size);
    m_header.last_table_block = 0;
    m_header.last_heap_block = 0;
    m_header.column_count = setup.columns.size();
    return {};
}

Util::OsErrorOr<void> EDBFile::write_header(Db::Core::TableSetup const& setup) {
    if (setup.columns.size() > 255) {
        return Util::OsError { .error = 0, .function = "Columns > 255 not supported" };
    }

    auto table_name = TRY(heap_allocate_and_get_span(setup.name.size()));
    std::copy(setup.name.begin(), setup.name.end(), reinterpret_cast<char*>(table_name.mapped_span.data()));

    EDBHeader header {
        .magic = {},
        .version = CurrentVersion,
        .block_size = m_header.block_size,
        .row_count = 0,
        .column_count = static_cast<uint8_t>(setup.columns.size()),
        .last_row_ptr = { 1, 0 },
        .last_table_block = 1,
        .last_heap_block = 2,
        .table_name = table_name.heap_span,
        .check_statement = {},           // TODO
        .auto_increment_value_count = 0, // TODO
        .key_count = 0,                  // TODO
    };

    auto stream = Util::WritableFileStream::borrow_fd(m_file.fd());
    TRY(stream.seek(0, Util::SeekDirection::FromStart));
    Util::Writer writer { stream };
    std::copy(std::begin(Magic), std::end(Magic), header.magic);
    TRY(writer.write_struct(header));

    for (auto const& column : setup.columns) {
        TRY(Serializer::write_column(*this, writer, column));
    }

    // TODO: AI
    // TODO: Keys

    return {};
}

Util::OsErrorOr<void> EDBFile::read_header() {
    struct stat stat;
    if (::fstat(m_file.fd(), &stat) < 0) {
        return Util::OsError { .error = errno, .function = "EDBFile: read_header: stat" };
    }
    m_file_size = stat.st_size;

    auto stream = Util::ReadableFileStream::borrow_fd(m_file.fd());
    TRY(stream.seek(0, Util::SeekDirection::FromStart));
    Util::BinaryReader reader { stream };
    m_header = TRY(reader.read_struct<EDB::EDBHeader>());
    m_block_count = (m_file_size - header_size()) / block_size() + 1;

    for (size_t s = 0; s < m_header.column_count; s++) {
        m_columns.push_back(TRY(reader.read_struct<Column>()));
    }

    return {};
}

Util::OsErrorOr<void> EDBFile::flush_header() {
    auto stream = Util::WritableFileStream::borrow_fd(m_file.fd());
    TRY(stream.seek(0, Util::SeekDirection::FromStart));
    TRY(Util::Writer { stream }.write_struct(m_header));
    return {};
}

Util::OsErrorOr<void> EDBFile::insert(Core::Tuple const& tuple) {
    // fmt::print("===== Insert\n");

    // 1. Find free place in Table blocks
    std::optional<HeapPtr> place_for_allocation;
    if (m_header.row_count == 0) {
        place_for_allocation = { 1, sizeof(Block) + sizeof(Table::TableBlock) };
    }
    else {
        HeapPtr last_row_ptr { 1, sizeof(Block) + sizeof(Table::TableBlock) };
        while (true) {
            if (last_row_ptr.offset + sizeof(Table::RowSpec) + row_size() > block_size()) {
                // TODO: Skip blocks that are full (rows_in_block == 255)
                last_row_ptr.block = last_row_ptr.block + 1;
                last_row_ptr.offset = sizeof(Block) + sizeof(Table::TableBlock);
                if (last_row_ptr.block >= m_block_count) {
                    fmt::print("no free block :(\n");
                    break;
                }
            }
            // fmt::print("Last row: {}:{}\n", last_row_ptr.block, last_row_ptr.offset);
            auto row = access<Table::RowSpec>(last_row_ptr);
            if (!row->is_used) {
                place_for_allocation = last_row_ptr;
                break;
            }
            last_row_ptr = { last_row_ptr.block, last_row_ptr.offset + sizeof(Table::RowSpec) + row_size() };
        }
    }

    if (!place_for_allocation) {
        // 2. If there is no free place, allocate new block
        fmt::print("will need to allocate block\n");
        auto block = TRY(allocate_block(BlockType::Table));
        place_for_allocation = HeapPtr { block, sizeof(Block) + sizeof(Table::TableBlock) };
    }

    // fmt::print("Place for allocation: {}:{}\n", place_for_allocation->block, place_for_allocation->offset);

    // 3. Actually write row
    auto row = access<Table::RowSpec>(*place_for_allocation, sizeof(Table::RowSpec) + row_size());
    row->is_used = 1;
    row->next_row = {};

    {
        Util::WritableMemoryStream stream;
        Util::Writer writer { stream };
        TRY(Serializer::write_row(*this, writer, m_columns, tuple));
        std::copy(stream.data().begin(), stream.data().end(), row->row);
    }

    // 4. Point last row into the newly placed row, if it exists
    if (m_header.row_count > 0) {
        auto last_row = access<Table::RowSpec>(m_header.last_row_ptr);
        last_row->next_row = *place_for_allocation;
    }

    // 5. Update headers
    m_header.last_row_ptr = *place_for_allocation;
    m_header.row_count = m_header.row_count + 1;
    access<Table::TableBlock>({ place_for_allocation->block, sizeof(Block) })->rows_in_block++;
    TRY(flush_header());
    return {};
}

size_t EDBFile::row_size() const {
    size_t size = 0;
    for (auto const& column : m_columns) {
        if (!column.not_null) {
            size += 1;
        }
        size += value_size_for_type(static_cast<Core::Value::Type>(column.type));
    }
    return size;
}

Util::OsErrorOr<std::vector<Core::Column>> EDBFile::read_columns() const {
    std::vector<Core::Column> columns;
    for (auto const& column : m_columns) {
        columns.push_back(Core::Column {
            read_heap(column.column_name).decode().encode(),
            static_cast<Core::Value::Type>(column.type),
            static_cast<bool>(column.auto_increment),
            static_cast<bool>(column.unique),
            static_cast<bool>(column.not_null),
            {},
        });
    }
    return columns;
}

Util::OsErrorOr<HeapSpan> EDBFile::heap_allocate(size_t size) {
    // fmt::print("Dump before alloc({}):\n", size);
    // m_heap.dump();
    HeapSpan span { TRY(m_heap.alloc(size)), size };
    // fmt::print("Dump after alloc({}):\n", size);
    // m_heap.dump();
    return span;
}

}