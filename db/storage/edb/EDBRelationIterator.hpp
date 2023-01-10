#pragma once

#include <db/core/Relation.hpp>
#include <db/storage/edb/EDBFile.hpp>

namespace Db::Storage::EDB {

class EDBRelationIteratorImpl : public Core::RelationIteratorImpl {
public:
    explicit EDBRelationIteratorImpl(EDBFile& file)
        : m_file(file)
        , m_row_ptr { 1, sizeof(Block) + sizeof(Table::TableBlock) } { }

    virtual std::unique_ptr<Core::RowReference> next() override;

private:
    Util::OsErrorOr<std::unique_ptr<Core::RowReference>> next_impl();
    
    EDBFile& m_file;
    HeapPtr m_row_ptr;
};

}
