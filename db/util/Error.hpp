/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// TODO: Think about moving it to EssaGUI.
// Most of code here is taken from SerenityOS, but ported to STL.
// https://github.com/SerenityOS/serenity/blob/master/AK/Error.h
// https://github.com/SerenityOS/serenity/blob/master/AK/Try.h

#pragma once

#include <cassert>
#include <optional>
#include <type_traits>
#include <variant>

#define ALWAYS_INLINE [[gnu::always_inline]]

namespace Util {

template<typename T, typename ErrorType>
class [[nodiscard]] ErrorOr;

#define TRY(...)                                       \
    ({                                                 \
        auto _temporary_result = (__VA_ARGS__);        \
        if (_temporary_result.is_error()) [[unlikely]] \
            return _temporary_result.release_error();  \
        _temporary_result.release_value();             \
    })

template<typename T, typename ErrorType>
class [[nodiscard]] ErrorOr final : public std::variant<T, ErrorType> {
public:
    using Variant = std::variant<T, ErrorType>;

    template<typename U>
    ALWAYS_INLINE ErrorOr(U&& value) requires(!std::is_same_v<std::remove_cvref_t<U>, ErrorOr<T, ErrorType>>)
        : Variant(std::forward<U>(value)) {
    }

    T& value() {
        return std::get<T>(*this);
    }
    T const& value() const { return std::get<T>(*this); }
    ErrorType& error() { return std::get<ErrorType>(*this); }
    ErrorType const& error() const { return std::get<ErrorType>(*this); }

    bool is_error() const { return std::holds_alternative<ErrorType>(*this); }

    T release_value() { return std::move(value()); }
    ErrorType release_error() { return std::move(error()); }

    T release_value_but_fixme_should_propagate_errors() {
        assert(!is_error());
        return release_value();
    }
};

// Partial specialization for void value type
template<typename ErrorType>
class [[nodiscard]] ErrorOr<void, ErrorType> {
public:
    ErrorOr(ErrorType error)
        : m_error(std::move(error)) {
    }

    ErrorOr() = default;
    ErrorOr(ErrorOr&& other) = default;
    ErrorOr(ErrorOr const& other) = default;
    ~ErrorOr() = default;

    ErrorOr& operator=(ErrorOr&& other) = default;
    ErrorOr& operator=(ErrorOr const& other) = default;

    ErrorType& error() { return m_error.value(); }
    bool is_error() const { return m_error.has_value(); }
    ErrorType release_error() { return std::move(m_error.value()); }
    void release_value() { }

private:
    std::optional<ErrorType> m_error;
};

}
