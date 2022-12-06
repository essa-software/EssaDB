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
