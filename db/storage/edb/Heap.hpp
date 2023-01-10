#pragma once

#include <EssaUtil/Error.hpp>
#include <db/storage/edb/AlignedAccess.hpp>
#include <db/storage/edb/Definitions.hpp>

namespace Db::Storage::EDB {

class EDBFile;

uint8_t* deref_impl(HeapPtr, EDBFile& file);

template<class T>
class AlignedAccessedHeapPtr {
public:
    explicit AlignedAccessedHeapPtr(HeapPtr ptr);

    AlignedAccess<T> deref(EDBFile& file) const {
        return AlignedAccess<T>(deref_impl(m_ptr, file));
    }

    auto ptr() const { return m_ptr; }
    bool is_null() const { return m_ptr.is_null(); }

private:
    HeapPtr m_ptr;
};

namespace Data {

enum class Signature : uint32_t {
    Used = 0x2137D05A,       // The memory is allocated.
    Empty = 0xBEBEBEBE,      // The memory was never allocated and is available to use.
    EndEdge = 0xE57F402D,    // Signature of the last header in the heap block.
    Freed = 0x2137DEAD,      // The memory was freed.
    ScrubBytes = 0xDEDEDEDE, // There was previously a header, but it was removed (e.g. because of merge)
};

class HeapBlock {
public:
    void init(EDBFile&);
    Util::OsErrorOr<std::optional<uint32_t>> alloc(EDBFile&, size_t size);
    Util::OsErrorOr<void> free(uint32_t offset);
    void leak_check();
    void dump(EDBFile&, BlockIndex);

private:
    void place_edge_headers(EDBFile&);
    void ensure_top_level_block();
    Util::OsErrorOr<void> merge_and_cleanup(EDBFile&);
    size_t data_size(EDBFile&) const;

    uint8_t m_data[0];
};

class Heap {
public:
    explicit Heap(EDBFile& file)
        : m_file(file) { }

    Util::OsErrorOr<HeapPtr> alloc(size_t size);
    void dump() const;
    void leak_check() const;
    Util::OsErrorOr<void> free(HeapPtr);

private:
    EDBFile& m_file;
};

}

}
