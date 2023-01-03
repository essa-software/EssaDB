#pragma once

#include <EssaUtil/Error.hpp>
#include <span>
#include <utility>

namespace Db::Storage::EDB {

class MappedFile {
public:
    MappedFile(MappedFile const&) = delete;
    MappedFile& operator=(MappedFile const&) = delete;
    MappedFile(MappedFile&& other) { *this = std::move(other); }

    MappedFile& operator=(MappedFile&& other) {
        if (this == &other) {
            return *this;
        }
        this->~MappedFile();
        m_fd = std::exchange(other.m_fd, 0);
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_size = std::exchange(other.m_size, 0);
        return *this;
    }

    ~MappedFile();
    static Util::OsErrorOr<MappedFile> map(int fd, size_t size);

    Util::OsErrorOr<void> remap(size_t new_size);

    std::span<uint8_t const> data() const;
    std::span<uint8_t> data();

    void dump() const;

private:
    MappedFile() = default;

    int m_fd;
    void* m_ptr = nullptr;
    size_t m_size = 0;
};

}
