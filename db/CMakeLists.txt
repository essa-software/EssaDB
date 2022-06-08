add_library(
    essadb

    core/AST.cpp
    core/Database.cpp
    core/RowWithColumnNames.cpp
    core/SelectResult.cpp
    core/Table.cpp
    core/Value.cpp

    sql/Lexer.cpp
    sql/Parser.cpp
    sql/SQL.cpp
)

setup_target(essadb)
