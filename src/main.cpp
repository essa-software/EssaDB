#include <core/Database.hpp>

#include <iostream>

Db::Core::DbErrorOr<void> run_query(Db::Core::Database& db) {
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Db::Core::Column("id")));
    TRY(table->add_column(Db::Core::Column("number")));

    TRY(table->insert({ { "id", 0 }, { "number", 69 } }));
    TRY(table->insert({ { "id", 1 }, { "number", 2137 } }));

    auto rows = table->select();

    std::cout << "SELECT * FROM test" << std::endl;
    for (auto row : rows)
        std::cout << row << std::endl;

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
