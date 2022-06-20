add_library(
    essadb

    core/AST.cpp
    core/Database.cpp
    core/Function.cpp
    core/ResultSet.cpp
    core/RowWithColumnNames.cpp
    core/Table.cpp
    core/Tuple.cpp
    core/Value.cpp

    sql/Lexer.cpp
    sql/Parser.cpp
    sql/SQL.cpp
)

setup_target(essadb)
