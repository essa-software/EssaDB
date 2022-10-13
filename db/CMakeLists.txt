add_library(
    essadb

    core/ast/Expression.cpp
    core/ast/Select.cpp
    core/ast/SelectColumns.cpp
    core/ast/Statement.cpp
    core/ast/TableExpression.cpp
    core/AbstractTable.cpp
    core/Database.cpp
    core/Function.cpp
    core/ResultSet.cpp
    core/Select.cpp
    core/Table.cpp
    core/Tuple.cpp
    core/TupleFromValues.cpp
    core/Value.cpp

    sql/Lexer.cpp
    sql/Parser.cpp
    sql/SQL.cpp
)

essautil_setup_target(essadb)
target_link_libraries(essadb PUBLIC essautil)
