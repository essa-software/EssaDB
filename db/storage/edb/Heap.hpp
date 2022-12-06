#pragma once

#include <EssaUtil/Error.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <db/storage/edb/MappedFile.hpp>

namespace Db::Storage::EDB {

class EDBFile;

Util::OsErrorOr<MappedFile> deref_impl(HeapPtr, size_t size, EDBFile& file);

template<class T>
class MappedHeapPtr {
public:
    MappedHeapPtr(HeapPtr ptr);

    Util::OsErrorOr<Mapped<T>> deref(EDBFile& file) const {
        return Mapped<T>(TRY(deref_impl(m_ptr, sizeof(T), file)));
    }

    auto ptr() const { return m_ptr; }
    bool is_null() const { return m_ptr.is_null(); }

private:
    HeapPtr m_ptr;
};

namespace Data {

enum class Signature : uint32_t {
    Used = 0x2137D05A,       // The memory is allocated.
    Empty = 0x00000000,      // The memory was never allocated and is available to use.
    EndEdge = 0xE57F402D,    // Signature of the last header in the heap block.
    Freed = 0x2137DEAD,      // The memory was freed.
    BigBlock = 0xB16C8056,   // Signature of a big block (should not occur in HeapBlock)
    ScrubBytes = 0xDEDEDEDE, // There was previously a header, but it was removed (e.g. because of merge)
};

class HeapBlock {
public:
    HeapBlock(HeapPtr prev)
        : m_prev(prev)
        , m_next({}) {
        init();
    }

    Util::OsErrorOr<HeapPtr> alloc(EDBFile& edb_file, BlockIndex, size_t size);
    Util::OsErrorOr<void> free(EDBFile& edb_file, BlockIndex, HeapPtr addr);
    void leak_check();
    void dump();

private:
    void init();
    void place_edge_headers();
    void ensure_top_level_block();
    Util::OsErrorOr<void> merge_and_cleanup(EDBFile&);

    size_t data_size() const { return /*heap_block_size*/ 4096 - sizeof(void*) * 2; }

    MappedHeapPtr<HeapBlock> m_prev;
    MappedHeapPtr<HeapBlock> m_next;
    char m_data[];
};

class Heap {
public:
    explicit Heap(EDBFile& file)
        : m_file(file) { }

    HeapPtr alloc(size_t size);
    void dump() const;
    void leak_check() const;
    void free(HeapPtr);

private:
    EDBFile& m_file;
};

}

}
