#include <db/sql/SQL.hpp>
#include <tests/setup.hpp>

using namespace Db::Core;

DbErrorOr<Database> setup_db() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, [group] VARCHAR);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(1, 'A');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(2, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(3, 'B');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(4, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(null, 'A');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(5, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(6, 'A');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(7, 'B');"));
    return db;
}

DbErrorOr<void> aggregate_simple() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT COUNT(id) FROM test;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT COUNT(id) FROM test;")).to_int()), 7, "proper row count was returned"));
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT SUM(id) FROM test;")).to_int()), 28, "proper row sum was returned"));
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT MIN(id) FROM test;")).to_int()), 0, "proper row min was returned"));
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT MAX(id) FROM test;")).to_int()), 7, "proper row max was returned"));
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT AVG(id) FROM test;")).to_int()), 3, "proper row avg was returned"));
    return {};
}

DbErrorOr<void> aggregate_count_with_group_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT [group], COUNT(id) AS [COUNTED] FROM test GROUP BY [group];")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 3, "select result is truncated to specified value"));
    // TRY(TRY(Db::Sql::run_query(db, "SELECT SUM(id) FROM test GROUP BY [group];")).to_int());
    // TRY(TRY(Db::Sql::run_query(db, "SELECT MIN(id) FROM test GROUP BY [group];")).to_int());
    // TRY(TRY(Db::Sql::run_query(db, "SELECT MAX(id) FROM test GROUP BY [group];")).to_int());
    // TRY(TRY(Db::Sql::run_query(db, "SELECT AVG(id) FROM test GROUP BY [group];")).to_int());
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
        { "aggregate_count_with_group_by", aggregate_count_with_group_by },
    };
}
