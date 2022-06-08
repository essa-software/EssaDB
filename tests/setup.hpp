#pragma once

#include <db/core/Database.hpp>

#include <sstream>

Db::Core::DbErrorOr<void> expect(bool, std::string const&);

template<class T>
auto expect_equal(T const& lhs, T const& rhs, std::string const& message) {
    std::ostringstream msg;
    msg << message << " (lhs=" << lhs << ", rhs=" << rhs << ")";
    return expect(lhs == rhs, msg.str());
}

using TestFunc = Db::Core::DbErrorOr<void>();
