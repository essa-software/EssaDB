#pragma once

#include <string>
#include <util/Error.hpp>

namespace Db::Core {

class DbError {
public:
    DbError(std::string message)
        : m_message(std::move(message)) { }

    std::string message() const { return m_message; }

private:
    std::string m_message;
};

template<class T>
using DbErrorOr = Util::ErrorOr<T, DbError>;

}
