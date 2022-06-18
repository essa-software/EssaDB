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
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT, date TIME)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, date) VALUES (0, 69, 'test', 48, #1990-02-12#)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, date) VALUES (1, 2137, 65, #1990-02-12#)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, date) VALUES (2, null, 89, #1990-02-12#)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, date) VALUES (3, 420, 100, #1990-02-12#)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, date) VALUES (4, 69, 'test1', 122, #1990-02-12#)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, date) VALUES (5, 69, 'test2', 58, #1990-02-12#)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, date) VALUES (3, 420, 100, #1990-02-12#)"));
    return db;
}

DbErrorOr<void> select_simple_with_addition() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT number, number + 5 AS [ADDED] FROM test;")).to_select_result());
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(1).to_int()), 74, "Values added properly"));
    return {};
}

DbErrorOr<void> select_simple_with_subtraction() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT number, number - 5 AS [ADDED] FROM test;")).to_select_result());
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(1).to_int()), 64, "Values subtracted properly"));
    return {};
}

DbErrorOr<void> select_simple_with_multiplication() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT number, number * 5 AS [ADDED] FROM test;")).to_select_result());
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(1).to_int()), 345, "Values multiplied properly"));
    return {};
}

DbErrorOr<void> select_simple_with_division() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT number, number / 5 AS [ADDED] FROM test;")).to_select_result());
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(1).to_int()), 13, "Values divided properly"));
    return {};
}

DbErrorOr<void> select_simple_with_string_addition() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT string, string + ' ' + date AS [ADDED] FROM test;")).to_select_result());
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "select_simple_with_addition", select_simple_with_addition },
        { "select_simple_with_subtraction", select_simple_with_subtraction },
        { "select_simple_with_multiplication", select_simple_with_multiplication },
        { "select_simple_with_division", select_simple_with_division },
        { "select_simple_with_string_addition", select_simple_with_string_addition },
    };
}
