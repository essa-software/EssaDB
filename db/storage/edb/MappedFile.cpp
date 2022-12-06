#include "MappedFile.hpp"

#include <db/storage/edb/Definitions.hpp>
#include <sys/mman.h>
#include <unistd.h>

namespace Db::Storage::EDB {

Util::OsErrorOr<MappedFile> MappedFile::map(int fd, off_t offset_in_file, size_t size) {
    MappedFile file;
    file.m_fd = fd;
    size_t real_offset = offset_in_file & ~(getpagesize() - 1);
    file.m_data_offset = offset_in_file - real_offset;
    file.m_mapped_size = size + file.m_data_offset + getpagesize();
    //fmt::print("size={}, off={}\n", file.m_mapped_size, file.m_data_offset);
    auto ptr = mmap(nullptr, file.m_mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, real_offset);
    if (ptr == MAP_FAILED) {
        return Util::OsError { .error = errno, .function = "MappedFile::map" };
    }
    file.m_ptr = ptr;
    return file;
}

MappedFile::~MappedFile() {
    if (m_ptr) {
        msync(m_ptr, m_mapped_size, MS_SYNC);
        munmap(m_ptr, m_mapped_size);
    }
}

std::span<uint8_t const> MappedFile::data() const {
    return { reinterpret_cast<uint8_t const*>(m_ptr) + m_data_offset, m_mapped_size - m_data_offset };
}

std::span<uint8_t> MappedFile::data() {
    return { reinterpret_cast<uint8_t*>(m_ptr) + m_data_offset, m_mapped_size - m_data_offset };
}

}
