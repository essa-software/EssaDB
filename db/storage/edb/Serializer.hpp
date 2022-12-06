#pragma once

#include <EssaUtil/Stream.hpp>
#include <db/core/Column.hpp>
#include <db/core/Value.hpp>
#include <db/storage/edb/Definitions.hpp>

namespace Db::Storage::EDB {

namespace Serializer {

Util::OsErrorOr<void> write_column(Util::Writer&, Core::Column const&);
Util::OsErrorOr<void> write_row(Util::Writer& writer, std::vector<Column> const& columns, Core::Tuple const& tuple);

};

}
