#include "Heap.hpp"

#include "EDBFile.hpp"

namespace Db::Storage::EDB {

Util::OsErrorOr<MappedFile> deref_impl(HeapPtr ptr, size_t size, EDBFile& file) {
    return file.map_heap(ptr, size);
}

namespace Data {

struct HeapHeader {
    Signature signature;
    uint32_t size {};

    HeapHeader* next() const {
        return size == 0 ? nullptr : (HeapHeader*)((char*)this + size) + 1;
    }

    bool valid_signature() const {
        return signature == Signature::BigBlock
            || signature == Signature::Empty
            || signature == Signature::Used
            || signature == Signature::EndEdge
            || signature == Signature::Freed;
    }

    bool available() const {
        return signature == Signature::Empty
            || signature == Signature::Freed;
    }

    bool freed() const {
        return signature == Signature::Freed;
    }

    char const* signature_string() const {
        switch (signature) {
        case Signature::Used:
            return "USED";
        case Signature::Empty:
            return "EMPTY";
        case Signature::EndEdge:
            return "END_EDGE";
        case Signature::Freed:
            return "FREED";
        case Signature::BigBlock:
            return "BIG_BLOCK";
        case Signature::ScrubBytes:
            return "SCRUB_BYTES";
        default:
            return nullptr;
        }
    }

    void print_signature() const {
        if (signature_string())
            fmt::print("{}", signature_string());
        else
            fmt::print("?0x{}?", (int)signature);
    }
};

void HeapBlock::place_edge_headers() {
    new (m_data) HeapHeader { Signature::Empty, static_cast<uint32_t>(data_size() - sizeof(HeapHeader) * 2) };
    new (m_data + data_size() - sizeof(HeapHeader)) HeapHeader { Signature::EndEdge };
}

void HeapBlock::init() {
    place_edge_headers();

    // Initialize rest of heap with scrub bytes
    memset(m_data + sizeof(HeapHeader), 0xef, data_size() - sizeof(HeapHeader) * 2);
}

void HeapBlock::ensure_top_level_block() {
    if (m_next.is_null()) {
        fmt::print("TODO: ensure_top_level_block\n");
        // Request a new memory block from the OS
        // auto memory = (char*)mmap(nullptr, sizeof(HeapBlock), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        // if (!memory) {
        //     perror("HeapBlock::alloc: mmap");
        //     abort();
        // }
        // m_next = reinterpret_cast<HeapBlock*>(memory);
        // new (m_next) HeapBlock { this };
    }
}

Util::OsErrorOr<void> HeapBlock::merge_and_cleanup(EDBFile& edb_file) {
    // Merge adjacent blocks
    auto header = reinterpret_cast<HeapHeader*>(m_data);
    while (header) {
        if (header->available()) {
            auto base_header = header;
            size_t size = header->size;
            while (header) {
                auto next_header = header->next();
                if (next_header && next_header->available())
                    size += next_header->size + sizeof(HeapHeader);
                else {
                    header = next_header;
                    break;
                }
                header = next_header;
            }
            base_header->size = size;
        }
        else
            header = header->next();
    }

    // Remove the whole block if it is made up from just freed blocks
    header = reinterpret_cast<HeapHeader*>(m_data);
    if (header->available() && header->size == data_size() - sizeof(HeapHeader) * 2) {
        if (!m_prev.is_null()) {
            if (!m_next.is_null()) {
                TRY(m_next.deref(edb_file))->m_prev = m_prev;
            }
            TRY(m_prev.deref(edb_file))->m_next = m_next;

            // TODO: Deallocate the block in edb file

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // WARNING: This is very unsafe when done improperly.
            // Don't do ANYTHING after this instruction
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // deallocate_top_level_block(this);
            // if (munmap(this, sizeof(HeapBlock)) < 0) {
            //     perror("HeapBlock::free: munmap");
            //     abort();
            // }
        }
    }
    return {};
}

Util::OsErrorOr<HeapPtr> HeapBlock::alloc(EDBFile& edb_file, BlockIndex block_index, size_t size) {
    HeapHeader* header = reinterpret_cast<HeapHeader*>(&m_data);

    while (header) {
        // printf("HEADER %x size=%u @%p next @%p\n", (uint32_t)header->signature, header->size, header, header->next());
        if (!header->valid_signature()) {
            fmt::print("heap_alloc_impl: Invalid header signature {:x} at {:p}\n", (uint32_t)header->signature, fmt::ptr(header));
            return Util::OsError { .error = 0, .function = "EDB HeapBlock::alloc: invalid header signature" };
        }
        if (header->available()) {
            // Calculate total free size
            size_t free_size = header->size;
            // printf("free_size=%zu, header_size=%zu, required_size=%zu\n", free_size, sizeof(HeapHeader), size);
            if (free_size < size + sizeof(HeapHeader) * 2) {
                // skip and try next block
                // printf("Skipping\n");
            }
            else {
                // printf("Allocating %zu bytes\n", size);

                auto distance_to_next_header = header->size - size - sizeof(HeapHeader);

                // set old header to be used
                header->signature = Signature::Used;
                header->size = size;

                // create a new header after data
                auto new_header_address = header->next();
                new (new_header_address) HeapHeader { Signature::Empty, static_cast<uint32_t>(distance_to_next_header) };
                return HeapPtr {block_index, };
            }
        }

        // Check for overflow
        if (header->signature == Signature::EndEdge) {
            ensure_top_level_block();
            return TRY(m_next.deref(edb_file))->alloc(edb_file, size);
        }

        // Try out next header, if it exists.
        header = header->next();
    }
    fmt::print("HeapBlock::alloc: No suitable block found!\n");
    return HeapPtr {};
}

Util::OsErrorOr<void> HeapBlock::free(EDBFile& edb_file, HeapPtr addr) {
    if (addr < m_data || addr >= m_data + data_size()) {
        if (!m_next.is_null())
            TRY(m_next.deref(edb_file))->free(addr);
        else {
                fmt::print("HeapBlock::free: {:p} was not allocated on heap\n", addr);
                abort();
        }
        return;
    }

    auto header = TRY(edb_file.map_heap_object<HeapHeader>({ addr.block, addr.offset - sizeof(HeapHeader) }));
    if (header->freed()) {
        printf("HeapBlock::free: Block already freed\n");
        abort();
    }
    if (!header->valid_signature()) {
        printf("HeapBlock::free: Invalid header signature %x (addr=%p)\n", (uint32_t)header->signature, header);
        abort();
    }

    header->signature = Signature::Freed;
    merge_and_cleanup();
}

void HeapBlock::leak_check() {
    printf("HeapBlock: Starting leak check on heap %p\n", this);

    // TODO: Check also big blocks??
    HeapHeader* header = reinterpret_cast<HeapHeader*>(m_data);
    bool leak_found = false;
    while (header) {
        if (header->signature == Signature::Used) {
            if (header->size > 0) {
                printf("(Leak check) Leaked %u bytes at %p\n", header->size, header + 1);
                leak_found = true;
            }
            header = header->next();
            continue;
        }
        if (!header->valid_signature()) {
            printf("(Leak check) Heap corrupted at %p\n", header);
            leak_found = true;
            break;
        }
        header = header->next();
    }

    if (m_next)
        m_next->leak_check();
    if (!leak_found)
        printf("(Leak check) No leak found on heap %p. Congratulations!\n", this);
}

void HeapBlock::dump() {
    HeapHeader* header = reinterpret_cast<HeapHeader*>(m_data);

    printf(" :: Heap %p; next = %p\n", this, m_next);
    while (header) {
        if (!header->valid_signature()) {
            printf("(corrupted at %p, signature is %x)\n", header, (uint32_t)header->signature);
            abort();
        }

        printf("    * %zu +%u",
            (reinterpret_cast<size_t>(header) - reinterpret_cast<size_t>(m_data)),
            header->size);
        if (header->next())
            printf(" next: %lu (%p)", reinterpret_cast<size_t>(header->next()) - reinterpret_cast<size_t>(m_data), header->next());

        if (header->available())
            printf(" (available)");
        if (header->freed())
            printf(" (freed)");
        printf(" ");
        header->print_signature();

        printf(" :: addr: %p\n", (header + 1));
        header = header->next();
    }
    if (m_next)
        m_next->dump();
}

// This is a hack to enable lazy-construction of heap also allowing to
// call member functions of HeapBlock
void* Heap::alloc(size_t size, size_t align) {
    if (!g_heap_initialized) {
        new (&g_heap_storage) HeapBlock { nullptr };
        g_heap_initialized = true;
    }
    assert(g_heap_initialized);

    if (size > sizeof(HeapBlock) - sizeof(HeapHeader)) {
        // TODO: Big blocks
        fmt::print("TODO: Big blocks\n");
        return nullptr;
        // printf("my_malloc: Too big size: %zu, allocating big block!\n", size);
        // auto memory = (char*)mmap(nullptr, size + sizeof(HeapHeader), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        // if (!memory) {
        //     perror("my_malloc: mmap");
        //     return nullptr;
        // }
        // auto header = reinterpret_cast<HeapHeader*>(memory);
        // header->signature = Signature::BigBlock;
        // header->size = size + sizeof(HeapHeader);
        // return header + 1;
    }
    return g_heap_storage.alloc(size, align);
}

void Heap::dump() const {
    fmt::print("----- HEAP DUMP BEGIN -----\n");
    if (!g_heap_initialized) {
        fmt::print("(heap is not initialized)\n");
        return;
    }
    g_heap_storage.dump();
    fmt::print("----- HEAP DUMP END -----\n");
}

void Heap::leak_check() const {
    g_heap_storage.leak_check();
}

void Heap::free(HeapPtr addr) {
    HeapHeader* header = reinterpret_cast<HeapHeader*>(addr) - 1;
    if (header->signature == Signature::BigBlock) {
        // TODO
        fmt::print("Heap::free: Freeing big block\n");
        // munmap(header, header->size);
        return;
    }

    if (!g_heap_initialized) {
        fmt::print("Heap::free: Heap is not initialized\n");
        abort();
    }

    // my_heap_dump();
    g_heap_storage.free(addr);
}

}
}
