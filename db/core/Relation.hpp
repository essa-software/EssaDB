#pragma once

#include "Column.hpp"
#include "Tuple.hpp"

#include <memory>
#include <vector>

namespace Db::Core {

class RelationIteratorImpl {
public:
    virtual ~RelationIteratorImpl() = default;
    virtual std::optional<Tuple> next() = 0;
};

// Boldly copied from SerenityOS
template<typename T, typename... Ts>
inline constexpr bool IsOneOf = (std::is_same_v<T, Ts> || ...);

// clang-format off
    
class RelationIterator {
public:
    RelationIterator(std::unique_ptr<RelationIteratorImpl> impl)
        : m_impl(std::move(impl)) { }

    auto next() { return m_impl->next(); }

    template<class Callback>
    void for_each_row(Callback&& callback) {
        for (auto row = next(); row; row = next()) {
            callback(*row);
        }
    }

    template<class Callback>
    DbErrorOr<void> try_for_each_row(Callback&& callback) {
        for (auto row = next(); row; row = next()) {
            TRY(callback(*row));
        }
        return {};
    }

private:
    std::unique_ptr<RelationIteratorImpl> m_impl {};
};

// clang-format on

// A database thing that has columns and rows. Note that it *doesn't allow*
// modifying a table; iterator returns a tuple as a value (For example, join
// or subquery result cannot be modified).
// Note that Relation must know columns in advance (it needs them anyway to
// correctly interpret serialized data), but doesn't need to know rows (but
// needs to know their count)
class Relation {
public:
    virtual ~Relation() = default;

    virtual std::vector<Column> const& columns() const = 0;
    virtual RelationIterator rows() const = 0;
    virtual size_t size() const = 0;

    // Find tuple that which `column`-th value is equal to `value`.
    // By default, this just iterates over the table; this may be
    // optimized by indexes in the future.
    virtual std::optional<Tuple const*> find_first_matching_tuple(size_t column, Value value);

    struct ResolvedColumn {
        size_t index;
        Column const& column;
    };
    std::optional<ResolvedColumn> get_column(std::string const& name) const;
    std::optional<ResolvedColumn> get_column(std::string const& name, Table* table) const;

    void dump_structure() const;
};

// An abstract table iterator that iterates over a container
// of rows stored in memory.
template<class It>
requires(std::forward_iterator<It>) class MemoryBackedRelationIteratorImpl : public RelationIteratorImpl {
public:
    explicit MemoryBackedRelationIteratorImpl(It begin, It end)
        : m_begin(begin)
        , m_current(begin)
        , m_end(end) { }

    virtual std::optional<Tuple> next() override {
        if (m_current == m_end)
            return {};
        return *(m_current++);
    }

private:
    It m_begin;
    It m_current;
    It m_end;
};

}
