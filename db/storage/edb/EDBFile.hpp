#pragma once

#include <EssaUtil/Error.hpp>
#include <EssaUtil/Stream.hpp>
#include <EssaUtil/Stream/File.hpp>
#include <cstddef>
#include <db/core/Column.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <db/storage/edb/Heap.hpp>
#include <db/storage/edb/MappedFile.hpp>
#include <memory>

namespace Db::Storage::EDB {

template<class T>
T* data_cast(uint8_t data[]) {
    return reinterpret_cast<T*>(data);
}

class EDBFile {
public:
    EDBFile(EDBFile const&) = delete;

    struct TableSetup {
        std::string name;
        std::vector<Core::Column> columns;
    };

    static Util::OsErrorOr<std::unique_ptr<EDBFile>> initialize(std::string const& path, TableSetup);
    static Util::OsErrorOr<std::unique_ptr<EDBFile>> open(std::string const& path);

    Util::OsErrorOr<void> insert(Core::Tuple const& tuple);

    Util::OsErrorOr<std::vector<Core::Column>> read_columns() const;
    auto const& header() const { return m_header; }

    Util::OsErrorOr<MappedFile> map_heap(HeapPtr ptr, size_t size) {
        if (ptr.block == 0) {
            return Util::OsError { .error = 0, .function = "map_heap_at trying to map block 0" };
        }
        if (ptr.offset + size > block_size()) {
            return Util::OsError { .error = 0, .function = "map_heap_at outside of block size" };
        }
        return MappedFile::map(m_file.fd(), block_offset(ptr.block) + ptr.offset, size);
    }

    template<class T>
    Util::OsErrorOr<Mapped<T>> map_heap_object(HeapPtr ptr, size_t size = sizeof(T)) {
        return Mapped<T> { map_heap(ptr, size) };
    }

private:
    friend std::unique_ptr<EDBFile> std::make_unique<EDBFile>(Util::File&&);
    friend class EDBRelationIteratorImpl;

    EDBFile(Util::File f);

    size_t header_size() const;
    size_t block_size() const;
    size_t block_offset(BlockIndex) const;

    Util::OsErrorOr<void> read_header();
    Util::OsErrorOr<void> write_block_size(TableSetup const&);
    Util::OsErrorOr<void> write_header(TableSetup const&);
    Util::OsErrorOr<void> flush_header();

    // Add `blocks` blocks to file without initializing them.
    Util::OsErrorOr<void> expand(size_t blocks);

    // Find first free block or expand file if it is not possible (Max O(n))
    Util::OsErrorOr<BlockIndex> allocate_block(BlockType);

    size_t row_size() const;

    size_t block_count() const { return (m_file_size - header_size()) / block_size(); }

    Util::OsErrorOr<Util::Buffer> read_heap(HeapSpan) const;
    Util::OsErrorOr<HeapSpan> heap_allocate(size_t size);

    Util::OsErrorOr<std::pair<HeapSpan, MappedFile>> heap_allocate_and_map(size_t size) {
        auto span = TRY(heap_allocate(size));
        return std::pair(span, TRY(MappedFile::map(m_file.fd(), block_offset(span.offset.block) + span.offset.offset, span.size)));
    }

    EDBHeader m_header;
    std::vector<Column> m_columns;
    Data::Heap m_heap;
    Util::File m_file;
    size_t m_file_size = 0;
    BlockIndex m_block_count = 1;
};

}
