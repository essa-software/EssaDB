#pragma once

#include <db/core/Relation.hpp>
#include <db/core/ast/Expression.hpp>

namespace Db::Core {

// TODO: Compound keys
struct PrimaryKey {
    std::string local_column;
};

struct ForeignKey {
    std::string local_column;
    std::string referenced_table;
    std::string referenced_column;
};

// Relation that contains keys and indexes, so that it can optimize
// some accesses.
class IndexedRelation : public Relation {
public:
    void set_primary_key(std::optional<PrimaryKey> key) {
        m_primary_key = std::move(key);
    }

    void add_foreign_key(ForeignKey key) {
        m_foreign_keys.push_back(std::move(key));
    }

    void drop_foreign_key(std::string const& name) {
        std::erase_if(m_foreign_keys, [&](auto const& fk) { return fk.local_column == name; });
    }

    auto const& primary_key() const { return m_primary_key; }
    auto const& foreign_keys() const { return m_foreign_keys; }

private:
    std::optional<PrimaryKey> m_primary_key;
    std::vector<ForeignKey> m_foreign_keys;
};

}
