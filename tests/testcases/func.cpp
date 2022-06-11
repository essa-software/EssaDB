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
    auto table = TRY(db.table("test"));

    TRY(table->insert({ { "id", Value::create_int(0) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test") }, { "integer", Value::create_int(48) } }));
    TRY(table->insert({ { "id", Value::create_int(1) }, { "number", Value::create_int(2137) }, { "integer", Value::create_int(65) } }));
    TRY(table->insert({ { "id", Value::create_int(2) }, { "number", Value::null() }, { "integer", Value::create_int(89) } }));
    TRY(table->insert({ { "id", Value::create_int(3) }, { "number", Value::create_int(420) }, { "integer", Value::create_int(100) } }));
    TRY(table->insert({ { "id", Value::create_int(4) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test3") }, { "integer", Value::create_int(122) } }));
    TRY(table->insert({ { "id", Value::create_int(5) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test2") }, { "integer", Value::create_int(57) } }));
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
