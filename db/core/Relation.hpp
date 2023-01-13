#pragma once

#include "Column.hpp"
#include "Tuple.hpp"

#include <EssaUtil/Config.hpp>
#include <EssaUtil/Error.hpp>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>

namespace Db::Core {

class RowReference {
public:
    RowReference() = default;
    virtual ~RowReference() = default;
    virtual Tuple read() const = 0;
    virtual void write(Tuple const&) = 0;

    // Remove a row. This must NOT invalidate other references.
    virtual void remove() = 0;
    virtual std::unique_ptr<RowReference> clone() const = 0;
};

class RelationIteratorImpl {
public:
    virtual ~RelationIteratorImpl() = default;
    virtual std::unique_ptr<RowReference> next() = 0;
};

// Boldly copied from SerenityOS
template<typename T, typename... Ts>
inline constexpr bool IsOneOf = (std::is_same_v<T, Ts> || ...);

class RelationIterator {
public:
    explicit RelationIterator(std::unique_ptr<RelationIteratorImpl> impl)
        : m_impl(std::move(impl)) { }

    auto next() { return m_impl->next(); }

    template<class Callback>
    void for_each_row(Callback&& callback) {
        for (auto row = next(); row; row = next()) {
            callback(row->read());
        }
    }

    template<class Callback>
    auto try_for_each_row(Callback&& callback) -> decltype(callback(std::declval<Tuple>())) {
        for (auto row = next(); row; row = next()) {
            TRY(callback(row->read()));
        }
        return {};
    }

private:
    std::unique_ptr<RelationIteratorImpl> m_impl {};
};

class MutableRelationIterator {
public:
    explicit MutableRelationIterator(std::unique_ptr<RelationIteratorImpl> impl)
        : m_impl(std::move(impl)) { }

    auto next() { return m_impl->next(); }

    template<class Callback>
    void for_each_row_reference(Callback&& callback) {
        for (auto row = next(); row; row = next()) {
            callback(*row);
        }
    }

    template<class Callback>
    auto try_for_each_row_reference(Callback&& callback) -> decltype(callback(std::declval<RowReference&>())) {
        for (auto row = next(); row; row = next()) {
            TRY(callback(*row));
        }
        return {};
    }

private:
    std::unique_ptr<RelationIteratorImpl> m_impl {};
};

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
    virtual MutableRelationIterator writable_rows() = 0;
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

class MemoryBackedRelationIteratorImpl : public RelationIteratorImpl {
public:
    using List = std::list<Tuple>;
    using Iterator = List::const_iterator;

    explicit MemoryBackedRelationIteratorImpl(List const& list)
        : m_list(list)
        , m_current(list.begin()) { }

    class RowReferenceImpl : public RowReference {
    public:
        explicit RowReferenceImpl(Iterator it)
            : m_it(it) {
        }

        virtual Tuple read() const override {
            return *m_it;
        }
        virtual void write(Tuple const&) override {
            ESSA_UNREACHABLE;
        }
        virtual void remove() override {
            ESSA_UNREACHABLE;
        }
        virtual std::unique_ptr<RowReference> clone() const override {
            return std::make_unique<RowReferenceImpl>(*this);
        }

    private:
        Iterator m_it;
    };

    virtual std::unique_ptr<RowReference> next() override {
        if (m_current == m_list.end())
            return {};
        return std::make_unique<RowReferenceImpl>(m_current++);
    }

private:
    List const& m_list;
    Iterator m_current;
};

// An abstract table iterator that iterates over a container
// of rows stored in memory.
class MutableMemoryBackedRelationIteratorImpl : public RelationIteratorImpl {
public:
    using List = std::list<Tuple>;
    using Iterator = List::iterator;

    explicit MutableMemoryBackedRelationIteratorImpl(List& list)
        : m_list(list)
        , m_current(list.begin()) { }

    class RowReferenceImpl : public RowReference {
    public:
        explicit RowReferenceImpl(List& list, Iterator it)
            : m_list(list)
            , m_it(it) {
        }

        virtual Tuple read() const override {
            return *m_it;
        }
        virtual void write(Tuple const& tuple) override {
            *m_it = tuple;
        }
        virtual void remove() override {
            m_list.erase(m_it);
        }
        virtual std::unique_ptr<RowReference> clone() const override {
            return std::make_unique<RowReferenceImpl>(*this);
        }

    private:
        List& m_list;
        Iterator m_it;
    };

    virtual std::unique_ptr<RowReference> next() override {
        if (m_current == m_list.end())
            return {};
        return std::make_unique<RowReferenceImpl>(m_list, m_current++);
    }

private:
    List& m_list;
    Iterator m_current;
};

}
