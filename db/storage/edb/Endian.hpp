#pragma once

#include <EssaUtil/Endianness.hpp>
#include <EssaUtil/Stream/Writer.hpp>
#include <concepts>

namespace Db::Storage::EDB {

template<class T>
struct [[gnu::packed]] LittleEndian {
    LittleEndian() = default;

    LittleEndian(T v)
        : encoded_value(v) { }

    T value() const {
        return Util::convert_from_little_to_host_endian(encoded_value);
    };

    operator T() const { return value(); }

    void set_value(T t) {
        encoded_value = Util::convert_from_host_to_little_endian(t);
    }

    T encoded_value;
};

template<std::floating_point T>
requires(sizeof(T) == 4) struct LittleEndian<T> : public LittleEndian<uint32_t> {
};
template<std::floating_point T>
requires(sizeof(T) == 8) struct LittleEndian<T> : public LittleEndian<uint64_t> {
};

template<class T>
struct [[gnu::packed]] BigEndian {
    BigEndian() = default;

    BigEndian(T v)
        : encoded_value(v) { }

    T value() const {
        return Util::convert_from_big_to_host_endian(encoded_value);
    };

    operator T() const { return value(); }

    void set_value(T t) {
        encoded_value = Util::convert_from_host_to_big_endian(t);
    }

    T encoded_value;
};

template<std::floating_point T>
requires(sizeof(T) == 4) struct BigEndian<T> : public BigEndian<uint32_t> {
};
template<std::floating_point T>
requires(sizeof(T) == 8) struct BigEndian<T> : public BigEndian<uint64_t> {
};

}
