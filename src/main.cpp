#include <core/AST.hpp>
#include <core/Database.hpp>

#include <iostream>

namespace Db::Core {

DbErrorOr<void> run_query(Database& db) {
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Column("id", Value::Type::Int)));
    TRY(table->add_column(Column("number", Value::Type::Int)));
    TRY(table->add_column(Column("string", Value::Type::Varchar)));

    TRY(table->insert({ { "id", Value::create_int(0) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test") } }));
    TRY(table->insert({ { "id", Value::create_int(1) }, { "number", Value::create_int(2137) } }));
    TRY(table->insert({ { "id", Value::create_int(2) }, { "number", Value::null() } }));
    TRY(table->insert({ { "id", Value::create_int(3) }, { "number", Value::create_int(420) } }));

    std::cout << "SELECT * FROM test" << std::endl;
    TRY(AST::Select({}, "test", {}).execute(db)).repl_dump(std::cout);

    std::cout << "SELECT number, string FROM test" << std::endl;
    TRY(AST::Select({ { "number", "string" } }, "test", {}).execute(db)).repl_dump(std::cout);

    std::cout << "SELECT id, number FROM test WHERE id = 2" << std::endl;
    TRY(AST::Select(
            { { "id", "number" } },
            "test",
            AST::Filter { .column = "id", .operation = AST::Filter::Operation::Equal, .rhs = Value::create_int(2) })
            .execute(db))
        .repl_dump(std::cout);

    std::cout << "SELECT id FROM test WHERE id <= 2" << std::endl;
    TRY(AST::Select(
            { { "id" } },
            "test",
            AST::Filter { .column = "id", .operation = AST::Filter::Operation::LessEqual, .rhs = Value::create_int(2) })
            .execute(db))
        .repl_dump(std::cout);

    return {};
}

}

// main
int main() {
    Db::Core::Database db;
    auto maybe_error = Db::Core::run_query(db);
    if (maybe_error.is_error()) {
        auto error = maybe_error.release_error();
        std::cout << "Query failed: " << error.message() << std::endl;
    }
}
