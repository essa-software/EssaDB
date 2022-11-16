#include <EssaUtil/Stream/File.hpp>
#include <EssaUtil/Stream/MemoryStream.hpp>
#include <db/core/TupleFromValues.hpp>
#include <db/storage/Serializer.hpp>
#include <tests/setup.hpp>

using namespace Db::Core;
using namespace Db::Storage;

DbErrorOr<void> basic() {
    Database db;
    auto& table = db.create_table("test", {});
    TRY(table.add_column(Column { "id", Value::Type::Int, false, false, false }));
    TRY(table.add_column(Column { "float", Value::Type::Float, false, false, false }));
    TRY(table.add_column(Column { "bool", Value::Type::Bool, false, false, false }));
    TRY(table.insert_unchecked(Tuple({ Value::create_int(0x01234567), Value::create_float(1000.0), Value::create_bool(false) })));
    TRY(table.insert_unchecked(Tuple({ Value::create_int(0x89abcdef), Value::create_float(-1000.0), Value::create_bool(true) })));

    auto stream = MUST(Util::WritableFileStream::open("/tmp/essadb-storage-test.edb", { .truncate = true }));
    Util::Writer writer { stream };
    Serializer serializer { writer };
    MUST(serializer.write_table(table));
    return {};
}

std::map<std::string, TestFunc> get_tests() {
    return {
        { "basic", basic }
    };
}
