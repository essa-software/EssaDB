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
    sql/Printing.cpp
    sql/SQL.cpp
    sql/Select.cpp
    sql/ast/Expression.cpp
    sql/ast/Function.cpp
    sql/ast/Select.cpp
    sql/ast/SelectColumns.cpp
    sql/ast/Show.cpp
    sql/ast/Statement.cpp
    sql/ast/TableExpression.cpp

    storage/CSVFile.cpp
    storage/FileBackedTable.cpp
    storage/edb/Definitions.cpp
    storage/edb/EDBFile.cpp
    storage/edb/EDBRelationIterator.cpp
    storage/edb/Heap.cpp
    storage/edb/MappedFile.cpp
    storage/edb/Serializer.cpp
)

essautil_setup_target(essadb)
target_link_libraries(essadb PUBLIC essautil)
