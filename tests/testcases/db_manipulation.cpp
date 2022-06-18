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
    TRY(table.add_column(Column("id", Value::Type::Int, Column::AutoIncrement::No)));
    TRY(table.add_column(Column("number", Value::Type::Int, Column::AutoIncrement::No)));
    TRY(table.add_column(Column("string", Value::Type::Varchar, Column::AutoIncrement::No)));
    TRY(table.add_column(Column("integer", Value::Type::Int, Column::AutoIncrement::No)));

    // TRY(Db::Sql::run_query(db, "CREATE TABLE [mew_test] (id INT, number INT, string VARCHAR, integer INT)"));
    TRY(expect(db.exists("new_test"), "Table successfully created!"));
    TRY(Db::Sql::run_query(db, "INSERT INTO [new_test] (id, number, string, integer) SELECT * FROM test WHERE id < 2;"));

    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM [new_test];")).to_select_result());

    TRY(expect(result.rows().size() == 2, "all rows were returned"));
    TRY(expect(result.rows()[0].value_count() == 4, "rows have proper column count"));
    return {};
}

DbErrorOr<void> alter_table_add_column() {
    auto db = TRY(setup_db());
    auto result = TRY(Db::Sql::run_query(db, "ALTER TABLE test ADD new VARCHAR")).to_select_result();
    TRY(expect(TRY(db.table("test"))->columns().size() == 5, "Column added successfully!"));
    return {};
}

DbErrorOr<void> alter_table_drop_column() {
    auto db = TRY(setup_db());
    auto result = TRY(Db::Sql::run_query(db, "ALTER TABLE test DROP COLUMN string")).to_select_result();
    TRY(expect(TRY(db.table("test"))->columns().size() == 3, "Column deleted successfully!"));
    return {};
}

DbErrorOr<void> alter_table_alter_column() {
    auto db = TRY(setup_db());
    auto result = TRY(Db::Sql::run_query(db, "ALTER TABLE test ADD new VARCHAR")).to_select_result();
    result = TRY(Db::Sql::run_query(db, "ALTER TABLE test ALTER COLUMN new INT")).to_select_result();
    TRY(expect(TRY(db.table("test"))->columns().size() == 5, "Column added successfully!"));
    TRY(expect(TRY(db.table("test"))->columns()[4].type() == Value::Type::Int, "Type changed successfully!"));
    return {};
}

DbErrorOr<void> delete_with_where() {
    auto db = TRY(setup_db());
    auto result = TRY(Db::Sql::run_query(db, "DELETE FROM test WHERE string LIKE 'te*'"));
    TRY(expect(TRY(db.table("test"))->rows().size() == 3, "Rows deleted successfully!"));
    return {};
}

DbErrorOr<void> update_assignment() {
    auto db = TRY(setup_db());
    auto update_result = TRY(Db::Sql::run_query(db, "UPDATE test SET id = 5"));
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test;")).to_select_result());

    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(0).to_int()), 5, "Table updated successfully!"));
    return {};
}

DbErrorOr<void> update_addition() {
    auto db = TRY(setup_db());
    auto update_result = TRY(Db::Sql::run_query(db, "UPDATE test SET id = id + 5"));
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test;")).to_select_result());

    TRY(expect_equal<size_t>(TRY(result.rows()[5].value(0).to_int()), 10, "Table updated successfully!"));
    return {};
}

DbErrorOr<void> autoincrement() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT AUTO_INCREMENT, value INT)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (value) VALUES (1000)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, value) VALUES (5, 1001)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (value) VALUES (1002)"));
    // TODO: Check SELECT:
    // | id | value |
    // | 0  | 1000  |
    // | 5  | 1001  |
    // | 1  | 1002  |
    TRY(Db::Sql::run_query(db, "SELECT * FROM test;")).repl_dump(std::cout);
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "drop_table", drop_table },
        { "truncate_table", truncate_table },
        { "insert_into_with_select", insert_into_with_select },
        { "alter_table_add_column", alter_table_add_column },
        { "alter_table_drop_column", alter_table_drop_column },
        { "alter_table_alter_column", alter_table_alter_column },
        { "delete_with_where", delete_with_where },
        { "update_assignment", update_assignment },
        { "update_addition", update_addition },
        { "autoincrement", autoincrement }
    };
}
