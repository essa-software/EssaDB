#pragma once

#include <EssaUtil/Error.hpp>
#include <string>

namespace Db::Core {

class DbError {
public:
    DbError(std::string message, size_t token)
        : m_message(std::move(message))
        , m_token(token) { }

    std::string message() const { return m_message; }
    size_t token() const { return m_token; }

private:
    std::string m_message;
    size_t m_token {};
};

template<class T>
using DbErrorOr = Util::ErrorOr<T, DbError>;

class DbErrorAddToken {
public:
    DbErrorAddToken(size_t t)
        : m_token(t) { }

    DbError operator()(DbError&& d) {
        return DbError { d.message(), m_token };
    }

private:
    size_t m_token;
};

}
