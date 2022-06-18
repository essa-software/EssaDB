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
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(5, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(6, 'AA');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO test (id, [group]) VALUES(7, 'B');"));

    TRY(Db::Sql::run_query(db, "CREATE TABLE new (id INT, [group] VARCHAR);"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(1, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(2, 'AA');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(3, 'AA');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(4, 'B');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(null, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(5, 'B');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(6, 'C');"));
    TRY(Db::Sql::run_query(db, "INSERT INTO new (id, [group]) VALUES(7, 'C');"));
    return db;
}

DbErrorOr<void> multuiple_tables() {
    auto db = TRY(setup_db());
    TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test")).to_select_result()).dump(std::cout);
    std::cout << "===================================\n";
    TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM new")).to_select_result()).dump(std::cout);

    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "multuiple_tables", multuiple_tables },
    };
}
