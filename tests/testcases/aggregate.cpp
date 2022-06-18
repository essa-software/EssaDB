#include <db/sql/SQL.hpp>
#include <tests/setup.hpp>

using namespace Db::Core;

DbErrorOr<Database> setup_db() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, [group] VARCHAR);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(1, 'AA');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(2, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(3, 'B');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(4, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(null, 'AA');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(2, 'C');")); // duplicate to test group by without aggregation
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(6, 'AA');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(7, 'B');"));
    return db;
}

DbErrorOr<void> aggregate_simple() {
    auto db = TRY(setup_db());
    TRY(expect_equal<int>(TRY(TRY(Db::Sql::run_query(db, "SELECT COUNT(id) FROM test;")).to_int()), 7, "proper row count was returned"));
    TRY(expect_equal<float>(TRY(TRY(Db::Sql::run_query(db, "SELECT SUM(id) FROM test;")).to_float()), 25, "proper row sum was returned"));
    TRY(expect_equal<float>(TRY(TRY(Db::Sql::run_query(db, "SELECT MIN(id) FROM test;")).to_float()), 0, "proper row min was returned"));
    TRY(expect_equal<float>(TRY(TRY(Db::Sql::run_query(db, "SELECT MAX(id) FROM test;")).to_float()), 7, "proper row max was returned"));
    TRY(expect_equal<float>(TRY(TRY(Db::Sql::run_query(db, "SELECT AVG(id) FROM test;")).to_float()), 3.125, "proper row avg was returned"));
    return {};
}

DbErrorOr<void> group_by_without_aggregation() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, [group] FROM test GROUP BY id, [group];")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 7, "proper row count was returned"));
    return {};
}

DbErrorOr<void> group_by_without_aggregation_not_all_grouped_by() {
    auto db = TRY(setup_db());
    TRY(expect_error(Db::Sql::run_query(db, "SELECT id, [group] FROM test GROUP BY [group];"), "Column 'id' must be either aggregate or occur in GROUP BY clause"));
    return {};
}

DbErrorOr<void> aggregate_count_with_group_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group];")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 3, "Proper row count"));
    return {};
}

DbErrorOr<void> aggregate_count_with_group_by_and_order_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group];")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 3, "Proper row count"));
    return {};
}

DbErrorOr<void> aggregate_count_with_where_and_group_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test WHERE id < 6 GROUP BY [group];")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 3, "Proper row count"));
    return {};
}

DbErrorOr<void> aggregate_count_with_having_and_group_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT [group], COUNT(id) AS [COUNTED], LEN([group]) AS [len] FROM test GROUP BY [group] HAVING COUNT(id) = 2;")).to_select_result());
    result.dump(std::cout);
    TRY(expect_equal<size_t>(result.rows().size(), 2, "Proper row count"));
    return {};
}

DbErrorOr<void> aggregate_error_not_all_columns_aggregate() {
    auto db = TRY(setup_db());
    TRY(expect_error(Db::Sql::run_query(db, "SELECT COUNT(id), id FROM test;"), "Column 'id' must be either aggregate or occur in GROUP BY clause"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "aggregate_simple", aggregate_simple },
        { "aggregate_error_not_all_columns_aggregate", aggregate_error_not_all_columns_aggregate },
        { "aggregate_count_with_group_by", aggregate_count_with_group_by },
        { "aggregate_count_with_where_and_group_by", aggregate_count_with_where_and_group_by },
        { "aggregate_count_with_having_and_group_by", aggregate_count_with_having_and_group_by },
        { "group_by_without_aggregation", group_by_without_aggregation },
        { "group_by_without_aggregation_not_all_grouped_by", group_by_without_aggregation_not_all_grouped_by },
    };
}
