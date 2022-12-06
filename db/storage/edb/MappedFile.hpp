#pragma once

#include <EssaUtil/Error.hpp>
#include <span>
#include <utility>

namespace Db::Storage::EDB {

class MappedFile {
public:
    MappedFile(MappedFile const&) = delete;
    MappedFile(MappedFile&& other)
        : m_fd(std::exchange(other.m_fd, 0))
        , m_ptr(std::exchange(other.m_ptr, nullptr))
        , m_mapped_size(std::exchange(other.m_mapped_size, 0))
        , m_data_offset(std::exchange(other.m_data_offset, 0)) { }

    MappedFile& operator=(MappedFile&& other) {
        if (this == &other) {
            return *this;
        }
        this->~MappedFile();
        m_fd = std::exchange(other.m_fd, 0);
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_mapped_size = std::exchange(other.m_mapped_size, 0);
        m_data_offset = std::exchange(other.m_data_offset, 0);
        return *this;
    }

    ~MappedFile();
    static Util::OsErrorOr<MappedFile> map(int fd, off_t offset_in_file, size_t size);

    std::span<uint8_t const> data() const;
    std::span<uint8_t> data();

private:
    MappedFile() = default;

    int m_fd;
    void* m_ptr = nullptr;
    size_t m_mapped_size = 0;
    size_t m_data_offset = 0;
};

template<class T>
class Mapped {
public:
    explicit Mapped(MappedFile f)
        : m_mapped_file(std::move(f)) { }

    static Util::OsErrorOr<Mapped<T>> map(int fd, off_t offset_in_file, size_t size = sizeof(T)) {
        return Mapped<T>(TRY(MappedFile::map(fd, offset_in_file, size)));
    }

    T* ptr() { return reinterpret_cast<T*>(m_mapped_file.data().data()); }
    T const* ptr() const { return reinterpret_cast<T const*>(m_mapped_file.data().data()); }

    T& operator*() { return *ptr(); }
    T const& operator*() const { return *ptr(); }

    T* operator->() { return ptr(); }
    T const* operator->() const { return ptr(); }

private:
    MappedFile m_mapped_file;
};

}
