#include "setup.hpp"

#include <db/util/Error.hpp>

using namespace Db::Core;

DbErrorOr<void> setup_db(Db::Core::Database& db) {

    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Column("id", Value::Type::Int)));
    TRY(table->add_column(Column("number", Value::Type::Int)));
    TRY(table->add_column(Column("string", Value::Type::Varchar)));

    TRY(table->insert({ { "id", Value::create_int(0) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test") } }));
    TRY(table->insert({ { "id", Value::create_int(1) }, { "number", Value::create_int(2137) } }));
    TRY(table->insert({ { "id", Value::create_int(2) }, { "number", Value::null() } }));
    TRY(table->insert({ { "id", Value::create_int(3) }, { "number", Value::create_int(420) } }));
    TRY(table->insert({ { "id", Value::create_int(4) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test3") } }));
    TRY(table->insert({ { "id", Value::create_int(5) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test2") } }));

    return {};
}

Db::Core::DbErrorOr<void> expect(bool b, std::string const& message) {
    if (!b)
        return DbError { message };
    return {};
}
