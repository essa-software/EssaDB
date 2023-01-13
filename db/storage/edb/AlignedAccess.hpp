#pragma once

#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <utility>

namespace Db::Storage::EDB {

// RAII wrapper that ensures that all accesses to T are aligned. This is done
// by just copying the object and "flushing" it to the original address in
// destructor.
template<class T>
class AlignedAccess {
public:
    explicit AlignedAccess(uint8_t* ptr)
        : m_ptr(ptr) {
        std::copy(ptr, ptr + sizeof(T), reinterpret_cast<uint8_t*>(&m_object));
    }

    explicit AlignedAccess(T* ptr)
        : m_ptr(ptr) {
        std::copy(ptr, ptr + 1, reinterpret_cast<uint8_t*>(&m_object));
    }

    ~AlignedAccess() {
        if (m_ptr) {
            flush();
        }
    }

    AlignedAccess(AlignedAccess const&) = delete;
    AlignedAccess& operator=(AlignedAccess const&) = delete;

    AlignedAccess(AlignedAccess&& other) {
        *this = std::move(other);
    }

    AlignedAccess& operator=(AlignedAccess&& other) {
        if (this == &other) {
            return *this;
        }
        flush();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_object = other.m_object;
        return *this;
    }

    T* operator->() { return &m_object; }
    T const* operator->() const { return &m_object; }
    T& operator*() { return m_object; }
    T const& operator*() const { return m_object; }

    uint8_t* original_ptr() const { return m_ptr; }

    void flush() {
        // fmt::print("AlignedAccess: Flushing {} bytes to {}\n", sizeof(T), fmt::ptr(m_ptr));
        std::copy(reinterpret_cast<uint8_t const*>(&m_object), reinterpret_cast<uint8_t const*>(&m_object) + sizeof(T), m_ptr);
    }

    void clear() {
        m_ptr = nullptr;
    }

private:
    uint8_t* m_ptr = nullptr;
    T m_object;
};

// AlignedAccess, but it allows you to specify dynamic size. This requires
// it to allocate on heap.
template<class T>
class AllocatingAlignedAccess {
public:
    explicit AllocatingAlignedAccess(uint8_t* ptr, size_t size)
        : m_ptr(ptr)
        , m_size(size) {
        m_object = reinterpret_cast<T*>(malloc(size));
        std::copy(ptr, ptr + size, reinterpret_cast<uint8_t*>(m_object));
    }

    ~AllocatingAlignedAccess() {
        if (m_ptr) {
            flush();
            free(m_object);
        }
    }

    AllocatingAlignedAccess(AllocatingAlignedAccess const&) = delete;
    AllocatingAlignedAccess& operator=(AllocatingAlignedAccess const&) = delete;

    AllocatingAlignedAccess(AllocatingAlignedAccess&& other) {
        *this = std::move(other);
    }

    AllocatingAlignedAccess& operator=(AllocatingAlignedAccess&& other) {
        if (this == &other) {
            return *this;
        }
        this->~AllocatingAlignedAccess();
        m_ptr = std::exchange(other.m_ptr, nullptr);
        m_object = std::exchange(other.m_object, nullptr);
        m_size = std::exchange(other.m_size, 0);
        return *this;
    }

    T* operator->() { return m_object; }
    T const* operator->() const { return m_object; }
    T& operator*() { return *m_object; }
    T const& operator*() const { return *m_object; }

    uint8_t* original_ptr() const { return m_ptr; }

    void flush() {
        // fmt::print("AllocatingAlignedAccess: Flushing {} bytes to {}\n", m_size, fmt::ptr(m_ptr));
        std::copy(reinterpret_cast<uint8_t const*>(m_object), reinterpret_cast<uint8_t const*>(m_object) + m_size, m_ptr);
    }

    void clear() {
        free(m_object);
        m_object = nullptr;
        m_ptr = nullptr;
        m_size = 0;
    }

private:
    uint8_t* m_ptr = nullptr;
    T* m_object = nullptr;
    size_t m_size = 0;
};

}
