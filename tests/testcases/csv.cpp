#include <tests/setup.hpp>

#include <db/core/AST.hpp>
#include <db/core/Database.hpp>
#include <db/core/SelectResult.hpp>
#include <db/sql/SQL.hpp>

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

DbErrorOr<void> csv_export_import() {
    auto db = TRY(setup_db());

    auto table = TRY(db.table("test"));
    table->export_to_csv("test.csv");

    db.create_table("test2");
    Table* new_table = TRY(db.table("test2"));
    TRY(new_table->import_from_csv("test.csv"));

    TRY(expect_equal(table->size(), new_table->size(), "original and imported tables have equal sizes"));
    TRY(expect(table->columns()[0].name() == "number" && table->columns()[1].name() == "string", "columns have proper names"));

    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "csv_export_import", csv_export_import },
    };
}
