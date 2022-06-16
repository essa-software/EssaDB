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
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT, [to_trim] VARCHAR)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (0, 69, 'test', 48, '   123   456   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, [to_trim]) VALUES (1, 2137, 65, ' 12 34 56   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, [to_trim]) VALUES (2, null, 89, '  1 2  3   4   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer, [to_trim]) VALUES (3, 420, 100, '123 456   ')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (4, 69, 'test1', 122, '   123 456')"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer, [to_trim]) VALUES (5, 69, 'test2', 58, '123 456')"));
    return db;
}

DbErrorOr<void> select_function_len() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LEN(string) AS [STRING LEN] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    TRY(expect_equal<size_t>(TRY(result.rows()[0].value(1).to_int()), 4, "string length was returned"));
    return {};
}

DbErrorOr<void> select_function_charindex() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, CHARINDEX('st', string) AS [INDEX] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_char() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, CHAR(integer) AS [CHAR] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_upper() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, UPPER(string) AS [UPPER STRING] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_upper_lower() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LOWER(UPPER(string)) AS [STRING] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_concat() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, CONCAT(string, ' ', STR(number)) AS [STRING] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_ltrim() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LEN([to_trim]) AS [LENGTH], LEN(LTRIM([to_trim])) AS [TRIMMED LENGTH] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_rtrim() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LEN([to_trim]) AS [LENGTH], LEN(RTRIM([to_trim])) AS [TRIMMED LENGTH] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_trim() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LEN([to_trim]) AS [LENGTH], LEN(TRIM([to_trim])) AS [TRIMMED LENGTH], TRIM([to_trim]) AS [TRIMMED] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_replace() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, REPLACE(string, 'test', 'replaced') AS STRING, REPLACE([to_trim], '2', '69') AS [REPLACED] FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_replicate() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, REPLICATE(CONCAT(string, ', '), 3) AS STRING FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_reverse() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, REVERSE(TRIM([to_trim])) AS STRING FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_left_right() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, LEFT(string, 4) AS LEFT, RIGHT(string, 4) AS RIGHT FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_stuff() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, STUFF(string, 1, 2, ' replaced ') AS REPLACED FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_function_translate() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, TRANSLATE(STUFF(string, 1, 2, ' WORD WORD WORDWORD WORD '), 'WORD', 'TRANSLATION') AS TRANSLATED FROM test;")).to_select_result());
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
        { "select_function_concat", select_function_concat },
        { "select_function_ltrim", select_function_ltrim },
        { "select_function_rtrim", select_function_rtrim },
        { "select_function_trim", select_function_trim },
        { "select_function_replace", select_function_replace },
        { "select_function_replicate", select_function_replicate },
        { "select_function_reverse", select_function_reverse },
        { "select_function_left_right", select_function_left_right },
        { "select_function_stuff", select_function_stuff },
        { "select_function_translate", select_function_translate },
    };
}
