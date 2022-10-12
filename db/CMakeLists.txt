add_library(
    essadb

    core/AST.cpp
    core/AbstractTable.cpp
    core/Database.cpp
    core/Expression.cpp
    core/Function.cpp
    core/ResultSet.cpp
    core/Select.cpp
    core/Table.cpp
    core/TableExpression.cpp
    core/Tuple.cpp
    core/TupleFromValues.cpp
    core/Value.cpp

    sql/Lexer.cpp
    sql/Parser.cpp
    sql/SQL.cpp
)

essautil_setup_target(essadb)
target_link_libraries(essadb PUBLIC essautil)
