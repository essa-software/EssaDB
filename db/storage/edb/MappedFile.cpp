#include "MappedFile.hpp"

#include <EssaUtil/Config.hpp>
#include <db/storage/edb/Definitions.hpp>
#include <sys/mman.h>
#include <unistd.h>

namespace Db::Storage::EDB {

Util::OsErrorOr<MappedFile> MappedFile::map(int fd, size_t size) {
    // fmt::print("MappedFile::map(size={})\n", size);
    MappedFile file;
    file.m_fd = fd;
    file.m_size = size;
    if (size == 0) {
        return file;
    }
    // fmt::print("size = {}\n", size);
    auto ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        return Util::OsError { .error = errno, .function = "MappedFile::map" };
    }
    file.m_ptr = ptr;
    // file.dump();
    return file;
}

Util::OsErrorOr<void> MappedFile::remap(size_t new_size) {
    msync(m_ptr, m_size, MS_SYNC);
    munmap(m_ptr, m_size);
    // fmt::print("old size = {} new size = {}\n", m_size, new_size);
    auto ptr = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if (ptr == MAP_FAILED) {
        return Util::OsError { .error = errno, .function = "MappedFile::remap" };
    }
    // fmt::print("remap {}+{} -> {}+{}\n", fmt::ptr(m_ptr), m_size, fmt::ptr(ptr), new_size);
    // fmt::print("address range: {:p}..{:p}\n", fmt::ptr(ptr), fmt::ptr((uint8_t*)ptr + new_size));
    m_ptr = ptr;
    m_size = new_size;
    // dump();
    return {};
}

void MappedFile::dump() const {
    fmt::print("MappedFile[{} +{}]\n", fmt::ptr(m_ptr), m_size);
}

MappedFile::~MappedFile() {
    if (m_ptr) {
        msync(m_ptr, m_size, MS_SYNC);
        munmap(m_ptr, m_size);
    }
}

std::span<uint8_t const> MappedFile::data() const {
    return { reinterpret_cast<uint8_t const*>(m_ptr), m_size };
}

std::span<uint8_t> MappedFile::data() {
    return { reinterpret_cast<uint8_t*>(m_ptr), m_size };
}

}
