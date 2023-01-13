#pragma once

#include <EssaUtil/Error.hpp>
#include <EssaUtil/Stream.hpp>
#include <EssaUtil/Stream/File.hpp>
#include <cstddef>
#include <db/core/Column.hpp>
#include <db/core/TableSetup.hpp>
#include <db/storage/edb/AlignedAccess.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <db/storage/edb/Heap.hpp>
#include <db/storage/edb/MappedFile.hpp>
#include <memory>
#include <utility>

namespace Db::Storage::EDB {

class EDBFile {
public:
    EDBFile(EDBFile const&) = delete;

    static Util::OsErrorOr<std::unique_ptr<EDBFile>> initialize(std::string const& path, Db::Core::TableSetup);
    static Util::OsErrorOr<std::unique_ptr<EDBFile>> open(std::string const& path);

    Util::OsErrorOr<void> rename(std::string const& new_name);
    Util::OsErrorOr<void> insert(Core::Tuple const& tuple);
    Util::OsErrorOr<void> remove(HeapPtr row, HeapPtr prev_row);

    Util::OsErrorOr<std::vector<Core::Column>> read_columns() const;
    auto const& header() const { return m_header; }
    auto const& raw_columns() const { return m_columns; }

    size_t block_size() const;
    size_t row_size() const;

    uint8_t* heap_ptr_to_mapped_ptr(HeapPtr);
    uint8_t const* heap_ptr_to_mapped_ptr(HeapPtr) const;

    template<class T>
    AlignedAccess<T> access(HeapPtr ptr) {
        assert(!ptr.is_null());
        auto mapped_ptr = heap_ptr_to_mapped_ptr(ptr);
        // fmt::print(":: access: {}:{} +{} = {:x}\n", ptr.block, ptr.offset, sizeof(T), mapped_ptr - m_mapped_file.data().data());
        assert(mapped_ptr + sizeof(T) <= m_mapped_file.data().end().base());
        return AlignedAccess<T> { mapped_ptr };
    }

    template<class T>
    AllocatingAlignedAccess<T> access(HeapPtr ptr, size_t size) {
        assert(!ptr.is_null());
        auto mapped_ptr = heap_ptr_to_mapped_ptr(ptr);
        // fmt::print(":: allocating access: {}:{} +{} = {:x}\n", ptr.block, ptr.offset, size, mapped_ptr - m_mapped_file.data().data());
        // fmt::print("   address range: {}..{}\n", fmt::ptr(mapped_ptr), fmt::ptr(mapped_ptr + size));
        assert(mapped_ptr + size <= m_mapped_file.data().end().base());
        return AllocatingAlignedAccess<T> { mapped_ptr, size };
    }

    Util::Buffer read_heap(HeapSpan) const;

    void dump_blocks();

    Util::OsErrorOr<HeapSpan> heap_allocate(size_t size);

    Util::OsErrorOr<HeapSpan> copy_to_heap(std::string const& str) {
        auto span = TRY(heap_allocate_and_get_span(str.size()));
        std::copy(str.begin(), str.end(), span.mapped_span.begin());
        return span.heap_span;
    }

    struct HeapAllocationResult {
        HeapSpan heap_span;
        std::span<uint8_t> mapped_span;
    };

    Util::OsErrorOr<HeapAllocationResult> heap_allocate_and_get_span(size_t size) {
        auto span = TRY(heap_allocate(size));
        return HeapAllocationResult { span, { heap_ptr_to_mapped_ptr(span.offset), span.size } };
    }

    template<class T>
    Util::OsErrorOr<AllocatingAlignedAccess<T>> heap_allocate(size_t size) {
        auto addr = TRY(heap_allocate(size));
        return AllocatingAlignedAccess<T> { heap_ptr_to_mapped_ptr(addr.offset), addr.size };
    }

    Util::OsErrorOr<void> heap_free(HeapPtr);

    Core::Value read_edb_value(Core::Value::Type, Value const&) const;
    Util::OsErrorOr<Value> write_edb_value(Core::Value const&);

private:
    EDBFile(Util::File, MappedFile);

    size_t header_size() const;
    size_t block_offset(BlockIndex) const;

    Util::OsErrorOr<void> read_header();

    // Write enough header to make allocate_block() work.
    Util::OsErrorOr<void> write_header_first_pass(Db::Core::TableSetup const&);

    Util::OsErrorOr<void> write_header(Db::Core::TableSetup const&);
    Util::OsErrorOr<void> flush_header();

    // Add `blocks` blocks to file without initializing them.
    Util::OsErrorOr<void> expand(size_t blocks);

    // Find first free block or expand file if it is not possible (Max O(n))
    Util::OsErrorOr<BlockIndex> allocate_block(BlockType);

    size_t block_count() const { return (m_file_size - header_size()) / block_size(); }

    EDBHeader m_header;
    std::vector<Column> m_columns;
    Data::Heap m_heap { *this };
    MappedFile m_mapped_file;
    Util::File m_file;
    std::string m_file_path;
    size_t m_file_size = 0;
    BlockIndex m_block_count = 1;
};

}
