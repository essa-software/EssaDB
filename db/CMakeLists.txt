add_library(
    essadb

    core/Database.cpp
    core/Relation.cpp
    core/ResultSet.cpp
    core/Table.cpp
    core/Tuple.cpp
    core/TupleFromValues.cpp
    core/Value.cpp

    sql/Lexer.cpp
    sql/Parser.cpp
    sql/SQL.cpp
    sql/Select.cpp
    sql/ast/Expression.cpp
    sql/ast/Function.cpp
    sql/ast/Select.cpp
    sql/ast/SelectColumns.cpp
    sql/ast/Show.cpp
    sql/ast/Statement.cpp
    sql/ast/TableExpression.cpp

    storage/Serializer.cpp
)

essautil_setup_target(essadb)
target_link_libraries(essadb PUBLIC essautil)
