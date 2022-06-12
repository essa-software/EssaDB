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

DbErrorOr<void> select_simple() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect(result.column_names() == std::vector<std::string> { "id", "number", "string", "integer", "date" }, "columns have proper names"));
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    TRY(expect(result.rows()[0].value_count() == 5, "rows have proper column count"));
    return {};
}

DbErrorOr<void> select_columns() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT number, string FROM test;")).to_select_result());
    TRY(expect(result.column_names() == std::vector<std::string> { "number", "string" }, "columns have proper names"));
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_order_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id AS [ID], number AS [NUMBER], string AS [STRING] FROM test ORDER BY string, number DESC;")).to_select_result());

    TRY(expect(TRY(result.rows()[1].value(1).to_string()) < TRY(result.rows()[5].value(1).to_string()), "values are sorted;"));
    return {};
}

DbErrorOr<void> select_order_by_desc() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id AS [ID], number AS [NUMBER], string AS [STRING] FROM test ORDER BY number DESC;")).to_select_result());

    TRY(expect(TRY(result.rows()[1].value(1).to_string()) > TRY(result.rows()[5].value(1).to_string()), "values are sorted"));
    return {};
}

DbErrorOr<void> select_top_number() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT TOP 2 * FROM test;")).to_select_result());
    TRY(expect(result.rows().size() == 2, "select result is truncated to specified value"));
    return {};
}

DbErrorOr<void> select_top_perc() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT TOP 75 PERC * FROM test;")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 5, "select result is truncated to specified value"));
    return {};
}

DbErrorOr<void> select_distinct() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT DISTINCT * FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 6, "select result is truncated to specified value"));
    return {};
}

DbErrorOr<void> select_with_aliases() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id AS ID, number AS NUM, string AS STR FROM test;")).to_select_result());
    TRY(expect(result.column_names() == std::vector<std::string> { "ID", "NUM", "STR" }, "columns have alias names"));
    TRY(expect(result.rows().size() == 7, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_aliases_with_square_brackets() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id AS [id], number AS [some number], string AS [ some  string ] FROM test;")).to_select_result());
    TRY(expect(result.column_names()[0] == "id", "square brackets are parsed properly"));
    TRY(expect(result.column_names()[1] == "some number", "spaces are recognized"));
    TRY(expect(result.column_names()[2] == "some  string", "spaces are not collapsed, but identifier is trimmed"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "select_simple", select_simple },
        { "select_columns", select_columns },
        { "select_order_by", select_order_by },
        { "select_order_by_desc", select_order_by_desc },
        { "select_top_number", select_top_number },
        { "select_top_perc", select_top_perc },
        { "select_distinct", select_distinct },
        { "select_with_aliases", select_with_aliases },
        { "select_aliases_with_square_brackets", select_aliases_with_square_brackets },
    };
}
