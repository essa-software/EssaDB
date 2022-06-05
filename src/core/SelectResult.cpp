#include "SelectResult.hpp"

#include "Table.hpp"

#include <cassert>

namespace Db::Core {

SelectIterator& SelectIterator::operator++() {
    assert(m_select_result);

    // TODO: find_next_row
    ++m_current_row;

    if (m_current_row >= m_select_result->m_table.size())
        m_is_end = true;
    return *this;
}

RowWithColumnNames SelectIterator::operator*() const {
    assert(m_select_result);
    return { m_select_result->m_table.m_rows[m_current_row], m_select_result->m_table };
}

}
