#pragma once

#include <db/core/ast/Statement.hpp>

namespace Db::Core::AST {

class Show : public Statement {
public:
    enum class Type {
        Tables,
    };

    explicit Show(ssize_t start, Type type)
        : Statement(start)
        , m_type(type) { }

    virtual DbErrorOr<ValueOrResultSet> execute(Database&) const override;

private:
    Type m_type;
};

}
