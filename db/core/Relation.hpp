#pragma once

#include "Column.hpp"
#include "Tuple.hpp"

#include <memory>
#include <vector>

namespace Db::Core {

class TupleWriter {
public:
    virtual ~TupleWriter() = default;
    virtual DbErrorOr<void> write(Tuple const&) const = 0;
    virtual Tuple read() const = 0;
    virtual DbErrorOr<void> remove() const = 0;
};

class RelationRowIteratorImpl {
public:
    virtual ~RelationRowIteratorImpl() = default;
    virtual std::optional<Tuple> next() = 0;
};

class WritableRelationRowIteratorImpl {
public:
    virtual ~WritableRelationRowIteratorImpl() = default;
    virtual std::optional<std::unique_ptr<TupleWriter>> next_writable() = 0;
};

// Boldly copied from SerenityOS
template<typename T, typename... Ts>
inline constexpr bool IsOneOf = (std::is_same_v<T, Ts> || ...);

// clang-format off
    
template<bool Const>
class RelationRowIterator {
public:
    RelationRowIterator(std::unique_ptr<RelationRowIteratorImpl> impl) requires(Const)
        : m_impl(std::move(impl)) { }

    RelationRowIterator(std::unique_ptr<WritableRelationRowIteratorImpl> impl) requires(!Const)
        : m_impl(std::move(impl)) { }

    auto next() requires(Const) { return m_impl->next(); }
    auto next_writable() requires(!Const) { return m_impl->next_writable(); }
    auto size() const { return m_impl->size(); }

    template<class Callback>
    requires(Const)
    void for_each_row(Callback&& callback) {
        for (auto row = next(); row; row = next()) {
            callback(*row);
        }
    }

    template<class Callback>
    requires(Const)
    DbErrorOr<void> try_for_each_row(Callback&& callback) {
        for (auto row = next(); row; row = next()) {
            TRY(callback(*row));
        }
        return {};
    }

    template<class Callback>
    requires(!Const)
    void for_each_row(Callback&& callback) {
        for (auto row = next_writable(); row; row = next_writable()) {
            callback(**row);
        }
    }

    template<class Callback>
    requires(!Const)
    DbErrorOr<void> try_for_each_row(Callback&& callback) {
        for (auto row = next_writable(); row; row = next_writable()) {
            TRY(callback(**row));
        }
        return {};
    }

private:
    std::unique_ptr<std::conditional_t<Const, RelationRowIteratorImpl, WritableRelationRowIteratorImpl>> m_impl {};
};

// clang-format on

// A database thing that has columns and rows. Note that it *doesn't allow*
// modifying a table; iterator returns a tuple as a value (For example, join
// or subquery result cannot be modified).
// Note that Relation must know columns in advance (it need it anyway to
// correctly interpret serialized data), but doesn't need to know rows (but
// needs to know their count)
class Relation {
public:
    virtual ~Relation() = default;

    virtual std::vector<Column> const& columns() const = 0;
    virtual std::vector<Tuple> const& raw_rows() const = 0;
    virtual RelationRowIterator<true> rows() const = 0;
    virtual RelationRowIterator<false> rows_writable() = 0;
    virtual size_t size() const = 0;

    struct ResolvedColumn {
        size_t index;
        Column const& column;
    };
    std::optional<ResolvedColumn> get_column(std::string const& name) const;
    std::optional<ResolvedColumn> get_column(std::string const& name, Table* table) const;
};

// An abstract table iterator that iterates over a container
// of rows stored in memory.
template<class It>
requires(std::forward_iterator<It>) class MemoryBackedRelationIteratorImpl : public RelationRowIteratorImpl {
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

template<class Container>
class WritableMemoryBackedRelationIteratorImpl : public WritableRelationRowIteratorImpl {
public:
    using Iterator = decltype(Container {}.begin());

    explicit WritableMemoryBackedRelationIteratorImpl(Container& container)
        : m_container(container)
        , m_current(container.begin()) { }

    class _TupleWriter : public TupleWriter {
    public:
        explicit _TupleWriter(WritableMemoryBackedRelationIteratorImpl& base, Iterator it)
            : m_base(base)
            , m_iterator(it) { }

    private:
        virtual DbErrorOr<void> write(Tuple const& tuple) const override {
            *m_iterator = tuple;
            return {};
        }
        virtual Tuple read() const override {
            return *m_iterator;
        }
        virtual DbErrorOr<void> remove() const override {
            m_base.m_container.erase(m_iterator);
            m_base.m_previous_erased = true;
            return {};
        }

        WritableMemoryBackedRelationIteratorImpl& m_base;
        Iterator m_iterator;
    };

    virtual std::optional<std::unique_ptr<TupleWriter>> next_writable() override {
        if (m_previous_erased)
            --m_current;
        if (m_current == std::end(m_container))
            return {};
        auto tuple_writer = std::make_unique<_TupleWriter>(*this, m_current);
        ++m_current;
        if (m_previous_erased)
            m_previous_erased = false;
        return tuple_writer;
    }

private:
    friend class _TupleWriter;
    Container& m_container;
    Iterator m_current;
    bool m_previous_erased = false;
};

}
