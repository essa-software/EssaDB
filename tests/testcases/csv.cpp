#include <tests/setup.hpp>

using namespace Db::Core;

DbErrorOr<Database> setup_db() {
    Database db;
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Column("number", Value::Type::Int)));
    TRY(table->add_column(Column("string", Value::Type::Varchar)));

    TRY(table->insert({ { "number", Value::create_int(1234) }, { "string", Value::create_varchar("test") } }));
    TRY(table->insert({ { "number", Value::null() }, { "string", Value::create_varchar("test") } }));
    TRY(table->insert({ { "number", Value::create_int(4321) }, { "string", Value::null() } }));
    TRY(table->insert({ { "number", Value::null() }, { "string", Value::null() } }));
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
