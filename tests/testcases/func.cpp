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
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (0, 69, test, 48)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (1, 2137, 65)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (2, null, 89)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (3, 420, 100)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (4, 69, testa, 122)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (5, 69, testb, 58)"));
    return db;
}

DbErrorOr<void> select_function_len() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LEN(string) AS [STRING LEN] FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(1).to_int()), 4, "string length was returned"));
    return {};
}

DbErrorOr<void> select_function_charindex() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, CHARINDEX('st', string) AS [INDEX] FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_char() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, CHAR(integer) AS [CHAR] FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_upper() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, UPPER(string) AS [UPPER STRING] FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_upper_lower() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LOWER(UPPER(string)) AS [STRING] FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "select_function_len", select_function_len },
        { "select_function_charindex", select_function_charindex },
        { "select_function_char", select_function_char },
        { "select_function_upper", select_function_upper },
        { "select_function_upper_lower", select_function_upper_lower },
    };
}
