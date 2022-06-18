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
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (5, 0.0, 'test2', 58, '123 456')"));
    return db;
}

DbErrorOr<void> select_function_abs() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, ABS(number) AS [NUM], ABS(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_acos() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, ACOS(number) AS [NUM], ACOS(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_asin() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, ASIN(number) AS [NUM], ASIN(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_atan() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, ATAN(number) AS [NUM], ATAN(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_atan2() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, [ATN2](number, 1) AS [NUM], [ATN2](integer, 1) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_ceil() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, CEILING(number) AS [NUM], CEILING(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_cos() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, COS(number) AS [NUM], COS(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_cot() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, COT(number) AS [NUM], COT(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_deg() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, DEGREES(number) AS [NUM], DEGREES(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_exp() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, EXP(number) AS [NUM], EXP(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_floor() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, FLOOR(number) AS [NUM], FLOOR(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_log() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LOG(number) AS [NUM], LOG(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_log10() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, [LOG10](number) AS [NUM], [LOG10](integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_pow() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, POWER(number, 2) AS [NUM], POWER(integer, 2) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_rad() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, RADIANS(number) AS [NUM], RADIANS(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_round() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, ROUND(number) AS [NUM], ROUND(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_sign() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, SIGN(number) AS [NUM], SIGN(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_sin() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, SIN(number) AS [NUM], SIN(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_sqrt() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, SQRT(number) AS [NUM], SQRT(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_sqr() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, SQUARE(number) AS [NUM], SQUARE(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_tan() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, TAN(number) AS [NUM], TAN(integer) AS [INT] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

std::map<std::string, TestFunc> get_tests() {
    return {
        { "select_function_abs", select_function_abs },
        { "select_function_acos", select_function_acos },
        { "select_function_asin", select_function_asin },
        { "select_function_atan", select_function_atan },
        { "select_function_atan2", select_function_atan2 },
        { "select_function_ceil", select_function_ceil },
        { "select_function_cos", select_function_cos },
        { "select_function_cot", select_function_cot },
        { "select_function_deg", select_function_deg },
        { "select_function_exp", select_function_exp },
        { "select_function_floor", select_function_floor },
        { "select_function_log", select_function_log },
        { "select_function_log10", select_function_log10 },
        { "select_function_pow", select_function_pow },
        { "select_function_round", select_function_round },
        { "select_function_sign", select_function_sign },
        { "select_function_sin", select_function_sin },
        { "select_function_sqrt", select_function_sqrt },
        { "select_function_sqr", select_function_sqr },
        { "select_function_tan", select_function_tan },
    };
}
