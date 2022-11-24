#pragma once

#include <db/sql/ast/Statement.hpp>

namespace Db::Sql::AST {

class Show : public Statement {
public:
    enum class Type {
        Tables,
    };

    explicit Show(ssize_t start, Type type)
        : Statement(start)
        , m_type(type) { }

    virtual SQLErrorOr<Core::ValueOrResultSet> execute(Core::Database&) const override;

private:
    Type m_type;
};

}
