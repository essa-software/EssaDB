#pragma once

#include <EssaUtil/Stream.hpp>
#include <db/core/Column.hpp>
#include <db/core/Value.hpp>
#include <db/storage/edb/Definitions.hpp>

namespace Db::Storage::EDB {

class EDBFile;

namespace Serializer {

Util::OsErrorOr<void> write_column(EDBFile&, Util::Writer&, Core::Column const&);
Util::OsErrorOr<void> write_row(EDBFile&, Util::Writer& writer, std::vector<Column> const& columns, Core::Tuple const& tuple);

};

}
