#include "db/core/Column.hpp"
#include "db/core/Value.hpp"
#include <tests/setup.hpp>

#include <db/core/AST.hpp>
#include <db/core/Database.hpp>
#include <db/core/SelectResult.hpp>
#include <db/sql/SQL.hpp>

#include <iostream>
#include <string>
#include <utility>

using namespace Db::Core;

DbErrorOr<Database> setup_db() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (0, 69, 'test', 48)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (1, 2137, 65)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (2, null, 89)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (3, 420, 100)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (4, 69, 'test1', 122)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (5, 69, 'test2', 58)"));
    return db;
}

DbErrorOr<void> drop_table() {
    auto db = TRY(setup_db());
    auto result = TRY(Db::Sql::run_query(db, "DROP TABLE test;")).to_select_result();
    TRY(expect(!db.exists("test"), "Table successfully deleted!"));
    return {};
}

DbErrorOr<void> truncate_table() {
    auto db = TRY(setup_db());
    auto result = TRY(Db::Sql::run_query(db, "TRUNCATE TABLE test;")).to_select_result();
    TRY(expect(TRY(db.table("test"))->size() == 0, "Table successfully truncated!"));
    return {};
}

DbErrorOr<void> insert_into_with_select() {
    auto db = TRY(setup_db());
    auto& table = db.create_table("new_test");
    TRY(table.add_column(Column("id", Value::Type::Int)));
    TRY(table.add_column(Column("number", Value::Type::Int)));
    TRY(table.add_column(Column("string", Value::Type::Varchar)));
    TRY(table.add_column(Column("integer", Value::Type::Int)));

    // TRY(Db::Sql::run_query(db, "CREATE TABLE [mew_test] (id INT, number INT, string VARCHAR, integer INT)"));
    TRY(expect(db.exists("new_test"), "Table successfully created!"));
    TRY(Db::Sql::run_query(db, "INSERT INTO [new_test] (id, number, string, integer) SELECT * FROM test WHERE id < 2;"));

    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM [new_test];")).to_select_result());
    
    TRY(expect(result.rows().size() == 2, "all rows were returned"));
    TRY(expect(result.rows()[0].value_count() == 4, "rows have proper column count"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "drop_table", drop_table },
        { "truncate_table", truncate_table },
        { "insert_into_with_select", insert_into_with_select },
    };
}
