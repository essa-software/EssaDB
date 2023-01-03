#include "db/core/Column.hpp"
#include "db/sql/SQL.hpp"
#include <EssaUtil/Stream/File.hpp>
#include <EssaUtil/Stream/MemoryStream.hpp>
#include <db/core/TupleFromValues.hpp>
#include <db/storage/FileBackedTable.hpp>
#include <db/storage/edb/EDBFile.hpp>
#include <fmt/core.h>
#include <iostream>
#include <tests/setup.hpp>

using namespace Db::Core;

DbErrorOr<void> basic() {
    auto file = MUST(Db::Storage::EDB::EDBFile::initialize("/tmp/essadb.edb",
        {
            .name = "test_table",
            .columns = {
                Db::Core::Column { "some_id", Db::Core::Value::Type::Int, false, false, false },
                Db::Core::Column { "some_float", Db::Core::Value::Type::Float, false, true, true },
                Db::Core::Column { "some_varchar", Db::Core::Value::Type::Varchar, false, false, false },
            },
        }));

    auto table = MUST(Db::Storage::FileBackedTable::open("/tmp/essadb.edb"));
    for (size_t s = 0; s < 50; s++) {
        TRY(table->insert_unchecked({
            s % 2 == 0 ? Db::Core::Value::null() : Db::Core::Value::create_int(s),
            Db::Core::Value::create_float(123.456f),
            Db::Core::Value::create_varchar(fmt::format("Varchar test: {}", s)),
        }));
    }
    fmt::print("table name = {}\n", table->name());
    table->dump_structure();

    Db::Core::Database db;
    db.add_table(std::move(table));

    auto result = MUST(Db::Sql::run_query(db, "SELECT * FROM test_table"));
    result.repl_dump(std::cout, Db::Core::ResultSet::FancyDump::Yes);

    return {};
}

std::map<std::string, TestFunc> get_tests() {
    return {
        { "basic", basic }
    };
}
