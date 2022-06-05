#include <core/Database.hpp>

#include <iostream>

Db::Core::DbErrorOr<void> run_query(Db::Core::Database& db) {
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Db::Core::Column("id")));
    TRY(table->add_column(Db::Core::Column("number")));

    TRY(table->insert({ { "id", 0 }, { "number", 69 } }));
    TRY(table->insert({ { "id", 1 }, { "number", 2137 } }));
    TRY(table->insert({ { "id", 2 }, { "number", {} } }));
    TRY(table->insert({ { "id", 3 }, { "number", 420 } }));

    std::cout << "SELECT * FROM test" << std::endl;
    table->select().display();

    std::cout << "SELECT number FROM test" << std::endl;
    table->select({ .columns = { "number" } }).display();

    std::cout << "SELECT number FROM test WHERE id = 2" << std::endl;
    table->select({ .columns = { "number" }, .filter = Db::Core::Filter { .column = "id", .expected_value = 3 } }).display();

    return {};
}

// main
int main() {
    Db::Core::Database db;
    auto maybe_error = run_query(db);
    if (maybe_error.is_error()) {
        auto error = maybe_error.release_error();
        std::cout << "Query failed: " << error.message() << std::endl;
    }
}
