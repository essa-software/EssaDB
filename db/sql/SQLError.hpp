#pragma once

#include <cstddef>
#include <db/core/DbError.hpp>
#include <string>

namespace Db::Sql {

class SQLError {
public:
    SQLError(std::string message, size_t token)
        : m_message(std::move(message))
        , m_token(token) { }

    std::string message() const { return m_message; }
    size_t token() const { return m_token; }

private:
    std::string m_message;
    size_t m_token;
};

class DbToSQLError {
public:
    DbToSQLError(size_t t)
        : m_token(t) { }

    SQLError operator()(Core::DbError&& d) {
        return SQLError { d.message(), m_token };
    }

private:
    size_t m_token;
};

template<class T>
using SQLErrorOr = Util::ErrorOr<T, SQLError>;

}
