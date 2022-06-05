#pragma once

#include "RowWithColumnNames.hpp"

namespace Db::Core {

class SelectResult;
class Table;

class SelectIterator {
public:
    enum EndTag { End };

    SelectIterator(EndTag)
        : m_is_end(true) { }

    SelectIterator(SelectResult const& result)
        : m_select_result(&result) { }

    SelectIterator& operator++();
    RowWithColumnNames operator*() const;

    bool operator!=(SelectIterator const& other) const {
        return m_is_end != other.m_is_end;
    }

private:
    SelectResult const* m_select_result = nullptr;
    size_t m_current_row = 0;
    bool m_is_end = false;
};

class SelectResult {
public:
    SelectResult(Table const& table)
        : m_table(table) { }

    SelectIterator begin() const { return SelectIterator { *this }; }
    SelectIterator end() const { return SelectIterator { SelectIterator::End }; }

private:
    friend SelectIterator;

    Table const& m_table;
};

}
