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
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (0, 69, 'test', 48)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (1, 2137, 65)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (2, null, 89)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, integer) VALUES (3, 420, 100)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (4, 69, 'test1', 122)"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, number, string, integer) VALUES (5, 69, 'test2', 58)"));
    return db;
}

DbErrorOr<void> select_where() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE id = 2;")).to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    TRY(expect(result.rows()[0].value(1).type() == Value::Type::Null, "number is null as in table"));
    return {};
}

DbErrorOr<void> select_where_multiple_rules_and() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id FROM test WHERE id = 2 AND id = 3;")).to_select_result());
    TRY(expect(result.rows().size() == 0, "filters are mutually exclusive"));
    return {};
}

DbErrorOr<void> select_where_multiple_rules_or() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id FROM test WHERE id = 2 OR id = 3;")).to_select_result());
    TRY(expect(result.rows().size() == 2, "filters were applied properly"));
    return {};
}

DbErrorOr<void> select_where_multiple_rules_and_or() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id FROM test WHERE id = 2 OR id = 3 AND number = 420;")).to_select_result());
    TRY(expect(result.rows().size() == 2, "filters were applied properly"));
    return {};
}

DbErrorOr<void> select_where_with_function_len() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, string, LEN(string) FROM test WHERE LEN(string) = 4;")).to_select_result());
    TRY(expect(result.rows().size() == 1, "filters were applied properly"));
    return {};
}

DbErrorOr<void> select_where_between() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, number FROM test WHERE id BETWEEN 2 AND 4;")).to_select_result());
    TRY(expect(result.rows().size() == 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_where_in_statement() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, number FROM test WHERE id IN(1, 5);")).to_select_result());
    TRY(expect(result.rows().size() == 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_without_asterisks() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 'test?';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_prefix_asterisk() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE '*st?';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_hash_mark() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 'test#';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_specified_char() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 'test[12]';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_negated_specified_char() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 'test[!1]';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 1, "1 row returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_specified_char_range() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 'test[1-2]';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_suffix_asterisk() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 'te*';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_suffix_asterisk_and_in_statement() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT id, number, string FROM test WHERE string LIKE 'te*' AND id IN(1, 5);")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 1, "1 row returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_two_asterisks() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE '*st*';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_asterisk_in_between() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string LIKE 't*t#';")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_is_null() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string IS NULL;")).to_select_result());
    TRY(expect(result.rows().size() == 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_is_not_null() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test WHERE string IS NOT NULL;")).to_select_result());
    TRY(expect(result.rows().size() == 3, "3 rows returned"));
    return {};
}

std::map<std::string, TestFunc> get_tests() {
    return {
        { "select_where", select_where },
        { "select_where_multiple_rules_and", select_where_multiple_rules_and },
        { "select_where_multiple_rules_or", select_where_multiple_rules_or },
        { "select_where_multiple_rules_and_or", select_where_multiple_rules_and_or },
        { "select_where_with_function_len", select_where_with_function_len },
        { "select_where_between", select_where_between },
        { "select_where_in_statement", select_where_in_statement },
        { "select_where_like_without_asterisks", select_where_like_without_asterisks },
        { "select_where_like_with_prefix_asterisk", select_where_like_with_prefix_asterisk },
        { "select_where_like_with_suffix_asterisk", select_where_like_with_suffix_asterisk },
        { "select_where_like_with_two_asterisks", select_where_like_with_two_asterisks },
        { "select_where_like_with_asterisk_in_between", select_where_like_with_asterisk_in_between },
        { "select_where_like_with_hash_mark", select_where_like_with_hash_mark },
        { "select_where_like_with_specified_char", select_where_like_with_specified_char },
        { "select_where_like_with_negated_specified_char", select_where_like_with_negated_specified_char },
        { "select_where_like_with_specified_char_range", select_where_like_with_specified_char_range },
        { "select_where_like_with_suffix_asterisk_and_in_statement", select_where_like_with_suffix_asterisk_and_in_statement },
        // { "select_where_is_null", select_where_is_null },
        { "select_where_is_not_null", select_where_is_not_null },
    };
}
