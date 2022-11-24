#pragma once

#include <sys/types.h>

namespace Db::Sql::AST {

class ASTNode {
public:
    explicit ASTNode(ssize_t start)
        : m_start(start) { }

    auto start() const { return m_start; }

private:
    size_t m_start;
};

}
