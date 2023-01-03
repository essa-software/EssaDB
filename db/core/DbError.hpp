#pragma once

#include <EssaUtil/Error.hpp>
#include <string>

namespace Db::Core {

class DbError {
public:
    explicit DbError(std::string message)
        : m_message(std::move(message)) { }

    std::string message() const { return m_message; }

private:
    std::string m_message;
};

template<class T>
using DbErrorOr = Util::ErrorOr<T, DbError>;

}

template<>
struct fmt::formatter<Db::Core::DbError> : public fmt::formatter<std::string_view> {
    template<typename FormatContext>
    constexpr auto format(Db::Core::DbError const& p, FormatContext& ctx) const {
        return format_to(ctx.out(), "DbError: {}", p.message());
    }
};
