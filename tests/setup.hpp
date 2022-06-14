#pragma once

#include "db/core/DbError.hpp"
#include <db/core/Database.hpp>

#include <sstream>

Db::Core::DbErrorOr<void> expect(bool, std::string const&);

template<class T>
auto expect_equal(T const& lhs, T const& rhs, std::string const& message) {
    std::ostringstream msg;
    msg << message << " (lhs=" << lhs << ", rhs=" << rhs << ")";
    return expect(lhs == rhs, msg.str());
}

template<class T>
Db::Core::DbErrorOr<void> expect_error(T const& lhs, std::string const& error) {
    if (!lhs.is_error())
        return Db::Core::DbError { "Not an error", 0 };
    return expect_equal(lhs.error().message(), error, "Invalid error");
}

using TestFunc = Db::Core::DbErrorOr<void>();
