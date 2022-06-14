#include <db/sql/SQL.hpp>
#include <tests/setup.hpp>

using namespace Db::Core;

DbErrorOr<Database> setup_db() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(1);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(2);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(3);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(4);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(null);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(5);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(6);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id) VALUES(7);"));
    return db;
}

DbErrorOr<void> aggregate_simple() {
    auto db = TRY(setup_db());
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT COUNT(id) FROM test;")).to_int()), 7, "proper row count was returned"));
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT SUM(id) FROM test;")).to_int()), 28, "proper row sum was returned"));
    return {};
}

DbErrorOr<void> aggregate_error_not_all_columns_aggregate() {
    auto db = TRY(setup_db());
    TRY(expect_error(Db::Sql::run_query(db, "SELECT COUNT(id), id FROM test;"), "All columns must be either aggregate or non-aggregate"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "aggregate_simple", aggregate_simple },
        { "aggregate_error_not_all_columns_aggregate", aggregate_error_not_all_columns_aggregate },
    };
}
