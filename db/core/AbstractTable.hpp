#pragma once

#include "Column.hpp"
#include "Tuple.hpp"
#include "db/core/DbError.hpp"

#include <memory>
#include <vector>

namespace Db::Core {

class AbstractTableRowIteratorImpl {
public:
    virtual ~AbstractTableRowIteratorImpl() = default;
    virtual std::optional<Tuple> next() = 0;
    virtual size_t size() const = 0;
};

// A wrapper for iterator indirection, just so you can use '.' instead of '->'
class AbstractTableRowIterator {
public:
    AbstractTableRowIterator(std::unique_ptr<AbstractTableRowIteratorImpl> impl)
        : m_impl(std::move(impl)) { }

    auto next() { return m_impl->next(); }
    auto size() const { return m_impl->size(); }

    std::vector<Tuple> read_all();

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
    std::unique_ptr<AbstractTableRowIteratorImpl> m_impl {};
};

// A database thing that has columns and rows. Note that it *doesn't allow*
// modifying a table; iterator returns a tuple as a value (For example, join
// or subquery result cannot be modified).
// Note that AbstractTable must know columns in advance (it need it anyway to
// correctly interpret serialized data), but doesn't need to know rows (but
// needs to know their count)
class AbstractTable {
public:
    virtual ~AbstractTable() = default;

    virtual std::vector<Column> const& columns() const = 0;
    virtual AbstractTableRowIterator rows() const = 0;

    size_t size() const { return rows().size(); }

    struct ResolvedColumn {
        size_t index;
        Column const& column;
    };
    std::optional<ResolvedColumn> get_column(std::string const& name) const;
};

// An abstract table iterator that iterates over a container
// of rows stored in memory.
template<class It>
requires(std::forward_iterator<It>) class MemoryBackedAbstractTableIteratorImpl : public AbstractTableRowIteratorImpl {
public:
    explicit MemoryBackedAbstractTableIteratorImpl(It begin, It end)
        : m_begin(begin)
        , m_current(begin)
        , m_end(end) { }

    virtual std::optional<Tuple> next() override {
        if (m_current == m_end)
            return {};
        return *(m_current++);
    }

    virtual size_t size() const override {
        return m_end - m_begin;
    }

    auto iterator() const {
        return m_current;
    }

private:
    It m_begin;
    It m_current;
    It m_end;
};

}
