#include "EDBRelationIterator.hpp"
#include "db/core/Relation.hpp"
#include "db/storage/edb/Serializer.hpp"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/Stream/MemoryStream.hpp>
#include <db/storage/edb/Definitions.hpp>

namespace Db::Storage::EDB {

std::unique_ptr<Core::RowReference> EDBRelationIteratorImpl::next() {
    return next_impl().release_value_but_fixme_should_propagate_errors();
}

class EDBRowReference : public Core::RowReference {
public:
    explicit EDBRowReference(Core::Tuple tuple, HeapPtr ptr, EDBFile& file)
        : m_tuple(tuple)
        , m_row_ptr(ptr)
        , m_file(file) { }

    ~EDBRowReference() {
        auto row = m_file.access<Table::RowSpec>(m_row_ptr, m_file.row_size() + sizeof(Table::RowSpec));
        row->free_data(m_file).release_value_but_fixme_should_propagate_errors();
        Util::WritableMemoryStream stream;
        Util::Writer writer { stream };
        Serializer::write_row(m_file, writer, m_file.raw_columns(), m_tuple).release_value_but_fixme_should_propagate_errors();
        std::copy(stream.data().begin(), stream.data().end(), row->row);
    }

private:
    virtual Core::Tuple read() const override {
        return m_tuple;
    }
    virtual void write(Core::Tuple const& tuple) override {
        m_tuple = tuple;
    }
    virtual void remove() override {
        // TODO
    }

    Core::Tuple m_tuple;
    HeapPtr m_row_ptr;
    EDBFile& m_file;
};

Util::OsErrorOr<std::unique_ptr<Core::RowReference>> EDBRelationIteratorImpl::next_impl() {
    if (m_file.header().row_count == 0) {
        return std::unique_ptr<Core::RowReference> {};
    }
    if (m_row_ptr.block == 0) {
        return std::unique_ptr<Core::RowReference> {};
    }

    // fmt::print("Last row: {}:{}\n", m_last_row_ptr.block, m_last_row_ptr.offset);
    auto row_ptr = m_row_ptr;
    auto row = m_file.access<Table::RowSpec>(m_row_ptr, m_file.row_size() + sizeof(Table::RowSpec));
    // fmt::print("rowspec {} {} {:x}\n", row->is_used, row->next_row.offset, row->row[0]);
    m_row_ptr = row->next_row;

    Util::ReadableMemoryStream stream { { row->row, m_file.row_size() } };
    Util::BinaryReader writer { stream };

    std::vector<Core::Value> values;
    for (auto const& column : m_file.raw_columns()) {
        auto is_null = column.not_null ? false : TRY(writer.read_little_endian<uint8_t>());
        switch (static_cast<Core::Value::Type>(column.type)) {
        case Core::Value::Type::Null:
            ESSA_UNREACHABLE;
            break;
        case Core::Value::Type::Int: {
            auto i = TRY(writer.read_little_endian<uint32_t>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_int(i));
            break;
        }
        case Core::Value::Type::Float: {
            auto f = TRY(writer.read_little_endian<float>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_float(f));
            break;
        }
        case Core::Value::Type::Varchar: {
            auto span = TRY(writer.read_struct<HeapSpan>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_varchar(m_file.read_heap(span).decode().encode()));
            break;
        }
        case Core::Value::Type::Bool: {
            auto b = TRY(writer.read_little_endian<uint8_t>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_bool(b));
            break;
        }
        case Core::Value::Type::Time: {
            auto time = TRY(writer.read_struct<Date>());
            values.push_back(is_null ? Core::Value::null() : Core::Value::create_time(Core::Date { .year = time.year, .month = time.month, .day = time.day }));
            break;
        }
        }
    }

    // fmt::print("D: ");
    // for (auto const& v : values) {
    //     fmt::print("{} ", v.to_debug_string());
    // }
    // fmt::print("\n");

    return std::make_unique<EDBRowReference>(Core::Tuple { values }, row_ptr, m_file);
}

}
