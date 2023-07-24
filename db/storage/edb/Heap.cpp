#include "Heap.hpp"

#include "EDBFile.hpp"
#include "EssaUtil/Config.hpp"
#include "db/storage/edb/Definitions.hpp"

#include <algorithm>

namespace Db::Storage::EDB {

uint8_t* deref_impl(HeapPtr ptr, EDBFile& file) {
    return file.heap_ptr_to_mapped_ptr(ptr);
}

namespace Data {

struct HeapHeader {
    Signature signature;
    uint32_t size {};

    bool has_valid_signature() const {
        return signature == Signature::Empty
            || signature == Signature::Used
            || signature == Signature::EndEdge
            || signature == Signature::Freed;
    }

    bool is_available() const {
        return signature == Signature::Empty
            || signature == Signature::Freed;
    }

    bool is_freed() const {
        return signature == Signature::Freed;
    }

    std::string_view signature_string() const {
        switch (signature) {
        case Signature::Used:
            return "USED";
        case Signature::Empty:
            return "EMPTY";
        case Signature::EndEdge:
            return "END_EDGE";
        case Signature::Freed:
            return "FREED";
        case Signature::ScrubBytes:
            return "SCRUB_BYTES";
        default:
            return "UNKNOWN";
        }
    }
};

void HeapBlock::place_edge_headers(EDBFile& file) {
    AlignedAccess<HeapHeader> start { m_data };
    *start = HeapHeader { Signature::Empty, static_cast<uint32_t>(data_size(file) - sizeof(HeapHeader) * 2) };
    AlignedAccess<HeapHeader> end { m_data + data_size(file) - sizeof(HeapHeader) };
    *end = HeapHeader { Signature::EndEdge };
}

void HeapBlock::init(EDBFile& file) {
    place_edge_headers(file);

    // Initialize rest of heap with scrub bytes
    memset(m_data + sizeof(HeapHeader), 0xef, data_size(file) - sizeof(HeapHeader) * 2);
}

Util::OsErrorOr<void> HeapBlock::merge_and_cleanup(EDBFile& edb_file) {
    // TODO
    (void)edb_file;
    return {};
}

size_t HeapBlock::data_size(EDBFile& file) const {
    return file.block_size() - sizeof(HeapBlock) - sizeof(Block);
}

Util::OsErrorOr<std::optional<uint32_t>> HeapBlock::alloc(EDBFile& file, size_t size) {
    uint8_t* header_ptr = m_data;
    while (true) {
        // printf("HEADER %x size=%u @%p next @%p\n", (uint32_t)header->signature, header->size, header, header->next());
        AlignedAccess<HeapHeader> header { header_ptr };
        if (!header->has_valid_signature()) {
            fmt::print("heap_alloc_impl: Invalid header signature {:x}\n", (uint32_t)header->signature);
            return Util::OsError { .error = 0, .function = "Corruption: EDB HeapBlock::alloc: Invalid header signature" };
        }
        if (header->is_available()) {
            // Calculate total free size
            size_t free_size = header->size;
            if (free_size < size + sizeof(HeapHeader) * 2) {
                // Skip and try next block.
            }
            else {
                auto distance_to_next_header = header->size - size - sizeof(HeapHeader);

                // Set old header to be used.
                header->signature = Signature::Used;
                header->size = size;

                // Create a new header after data.
                AlignedAccess<HeapHeader> new_header { header_ptr + header->size + sizeof(HeapHeader) };
                *new_header = { Signature::Empty, static_cast<uint32_t>(distance_to_next_header) };
                return header_ptr - m_data + sizeof(HeapHeader);
            }
        }

        // Check for overflow
        if (header->signature == Signature::EndEdge) {
            fmt::print("EDB HeapBlock::alloc: Reached end edge\n");
            return std::optional<uint32_t> {};
        }

        // Try out next header, if it exists.
        header_ptr += header->size + sizeof(HeapHeader);
        if (header_ptr > m_data + data_size(file)) {
            return Util::OsError { .error = 0, .function = "Corruption: EDB HeapBlock::alloc: Out of range without end edge" };
        }
    }
}

Util::OsErrorOr<void> HeapBlock::free(uint32_t offset) {
    AlignedAccess<HeapHeader> header { m_data + offset - sizeof(HeapHeader) };
    header->signature = Signature::Freed;
    // TODO: Merge and cleanup
    return {};
}

void HeapBlock::leak_check() {
    fmt::print("TODO: Leak check\n");
}

void HeapBlock::dump(EDBFile& file, BlockIndex index) {
    fmt::print("HeapBlock {}\n", index);
    uint8_t* header_ptr = m_data;
    while (true) {
        AlignedAccess<HeapHeader> header { header_ptr };
        fmt::print("- {:05x}: ", header_ptr - m_data);
        if (!header->has_valid_signature()) {
            fmt::print("INVALID({:08x})", static_cast<uint32_t>(header->signature));
        }
        else {
            fmt::print("{} ", header->signature_string());
        }
        fmt::print(" size={}\n", header->size);
        header_ptr += header->size + sizeof(HeapHeader);
        if (header_ptr + sizeof(HeapHeader) > m_data + data_size(file)) {
            break;
        }
    }
}

Util::OsErrorOr<HeapPtr> Heap::alloc(size_t size) {
    if (size > m_file.block_size() - sizeof(HeapHeader)) {
        return Util::OsError { .error = 0, .function = "TODO: Big blocks" };
    }

    // First, try to allocate in every block in the list.
    {
        // Note: First heap block is always 2.
        size_t current_block = 2;
        while (true) {
            auto block = m_file.access<Block>(HeapPtr { current_block, 0 }, m_file.block_size());
            if (block->type != BlockType::Heap) {
                return Util::OsError { .error = 0, .function = "Corruption: Found non-heap block in heap block list" };
            }

            auto& heap_block = reinterpret_cast<HeapBlock&>(block->data);
            auto result = TRY(heap_block.alloc(m_file, size));
            if (result) {
                return HeapPtr { current_block, *result + sizeof(Block) };
            }
            // FIXME: Merge&cleanup here

            if (!block->next_block) {
                break;
            }
            current_block = block->next_block;
        }
    }

    // If there is free space in existing blocks, allocate a new block
    auto new_block_idx = TRY(m_file.allocate_block(BlockType::Heap));
    auto block = m_file.access<Block>(HeapPtr { new_block_idx, 0 }, m_file.block_size());
    auto& heap_block = reinterpret_cast<HeapBlock&>(block->data);
    auto result = TRY(heap_block.alloc(m_file, size));
    if (!result) {
        // We should always have space in a newly allocated block!
        ESSA_UNREACHABLE;
    }
    return HeapPtr { new_block_idx, *result + sizeof(Block) };
}

void Heap::dump() const {
    fmt::print("----- HEAP DUMP BEGIN -----\n");
    auto block = m_file.access<HeapBlock>(HeapPtr { 2, sizeof(Block) }, m_file.block_size() - sizeof(Block));
    block->dump(m_file, 2);
    fmt::print("----- HEAP DUMP END -----\n");
}

void Heap::leak_check() const {
    // TODO
}

Util::OsErrorOr<void> Heap::free(HeapPtr ptr) {
    auto block = m_file.access<HeapBlock>(HeapPtr { ptr.block, sizeof(Block) }, m_file.block_size() - sizeof(Block));
    TRY(block->free(ptr.offset - sizeof(Block)));
    block.flush();
    return {};
}

}

}
