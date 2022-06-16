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
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, number FLOAT, string VARCHAR, integer INT, [to_trim] VARCHAR)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (0, 69.05, 'test', 48, '   123   456   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, [to_trim]) VALUES (1, 2137.123, 65, ' 12 34 56   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, [to_trim]) VALUES (2, null, 89, '  1 2  3   4   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, [to_trim]) VALUES (3, -420.5, -100, '123 456   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (4, 69.21, 'test1', 122, '   123 456')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (5, 69.45, 'test2', 58, '123 456')"));
    return db;
}

DbErrorOr<void> select_function_abs() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, ABS(number) AS [NUM], ABS(integer) AS [INT] FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "select_function_abs", select_function_abs },
    };
}
