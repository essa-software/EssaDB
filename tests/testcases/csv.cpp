#include <tests/setup.hpp>

#include <db/core/Database.hpp>
#include <db/core/ResultSet.hpp>
#include <db/core/Table.hpp>
#include <db/sql/SQL.hpp>
#include <iostream>

using namespace Db::Core;

auto sql_to_db_error(Db::Sql::SQLError&& e) { return DbError { e.message() }; }

DbErrorOr<Database> setup_db() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, number INT, string VARCHAR, integer INT)").map_error(sql_to_db_error));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (0, 69, 'test', 48)").map_error(sql_to_db_error));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (1, 2137, 65)").map_error(sql_to_db_error));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (2, null, 89)").map_error(sql_to_db_error));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (3, 420, 100)").map_error(sql_to_db_error));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (4, 69, 'test1', 122)").map_error(sql_to_db_error));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (5, 69, 'test2', 58)").map_error(sql_to_db_error));
    return db;
}

DbErrorOr<void> csv_export_import() {
    auto db = TRY(setup_db());

    auto table = TRY(db.table("test"));
    table->export_to_csv("test.csv");

    auto* new_table = &db.create_table("newtest", nullptr);
    TRY(new_table->import_from_csv(db, "test.csv"));

    auto result = TRY(Db::Sql::run_query(db, "SELECT * FROM newtest;").map_error(sql_to_db_error)).as_result_set();

    TRY(expect_equal(table->size(), new_table->size(), "original and imported tables have equal sizes"));
    TRY(expect(result.column_names() == std::vector<std::string> { "id", "number", "string", "integer" }, "columns have proper names"));

    return {};
}

DbErrorOr<void> csv_export_import_with_aliases() {
    auto db = TRY(setup_db());

    auto result = TRY(Db::Sql::run_query(db, "SELECT id AS [ID], number AS [NUM], string AS [STR], integer AS [INT] FROM test;").map_error(sql_to_db_error)).as_result_set();
    result.dump(std::cout, Db::Core::ResultSet::FancyDump::Yes);
    auto table = TRY(db.create_table_from_query(result, "test_from_query"));
    table->export_to_csv("test.csv");

    auto& new_table = db.create_table("new_test", nullptr);
    TRY(new_table.import_from_csv(db, "test.csv"));

    result = TRY(Db::Sql::run_query(db, "SELECT * FROM [new_test];").map_error(sql_to_db_error)).as_result_set();
    result.dump(std::cout, Db::Core::ResultSet::FancyDump::Yes);

    TRY(expect_equal(table->size(), new_table.size(), "original and imported tables have equal sizes"));
    TRY(expect(result.column_names() == std::vector<std::string> { "ID", "NUM", "STR", "INT" }, "columns have proper names"));

    return {};
}

DbErrorOr<void> csv_import_statement() {
    auto db = TRY(setup_db());

    auto result = TRY(Db::Sql::run_query(db, "SELECT id AS [ID], number AS [NUM], string AS [STR], integer AS [INT] FROM test;").map_error(sql_to_db_error)).as_result_set();
    result.dump(std::cout, Db::Core::ResultSet::FancyDump::Yes);
    auto table = TRY(db.create_table_from_query(result, "test_from_query"));
    table->export_to_csv("test.csv");

    auto& new_table = db.create_table("new_test", nullptr);
    TRY(Db::Sql::run_query(db, "IMPORT CSV 'test.csv' INTO new_test").map_error(sql_to_db_error));

    result = TRY(Db::Sql::run_query(db, "SELECT * FROM [new_test];").map_error(sql_to_db_error)).as_result_set();
    result.dump(std::cout, Db::Core::ResultSet::FancyDump::Yes);

    TRY(expect_equal(table->size(), new_table.size(), "original and imported tables have equal sizes"));
    TRY(expect(result.column_names() == std::vector<std::string> { "ID", "NUM", "STR", "INT" }, "columns have proper names"));

    return {};
}

std::map<std::string, TestFunc> get_tests() {
    return {
        { "csv_export_import", csv_export_import },
        { "csv_export_import_with_aliases", csv_export_import_with_aliases },
        { "csv_import_statement", csv_import_statement }
    };
}
