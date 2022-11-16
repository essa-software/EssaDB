#pragma once

#include <EssaUtil/Stream.hpp>
#include <db/core/Column.hpp>
#include <db/core/Value.hpp>

namespace Db::Storage {

class Serializer {
public:
    explicit Serializer(Util::Writer& writer)
        : m_writer(writer) { }

    Util::OsErrorOr<void> write_table(Core::Table const&);

private:
    Util::OsErrorOr<void> write_column(Core::Column const&);
    Util::OsErrorOr<void> write_row(Core::Tuple const&);
    Util::OsErrorOr<void> write_value(Core::Value const&);

    Util::Writer& m_writer;
};

}
