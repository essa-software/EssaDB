#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <util/Error.hpp>

class DbError {
public:
    DbError(std::string message)
        : m_message(std::move(message)) { }

    std::string message() const { return m_message; }

private:
    std::string m_message;
};

template<class T>
using DbErrorOr = Util::ErrorOr<T, DbError>;

// NonCopyable
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(NonCopyable const&) = delete;
    NonCopyable& operator=(NonCopyable const&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};

// Value
using Value = std::optional<int>;

// Row
class Row {
public:
    Row(std::span<Value> values)
        : m_values(values.size()) {
        std::copy(values.begin(), values.end(), m_values.begin());
    }

    size_t value_count() const { return m_values.size(); }
    Value value(size_t index) const { return m_values[index]; }

    auto begin() const { return m_values.begin(); }
    auto end() const { return m_values.end(); }

private:
    std::vector<Value> m_values;
};

class Table;

class RowWithColumnNames {
public:
    using MapType = std::map<std::string, Value>;

    static DbErrorOr<RowWithColumnNames> from_map(Table const& table, MapType map);

    RowWithColumnNames(Row row, Table const& table)
        : m_row(std::move(row))
        , m_table(table) { }

    Row row() const { return m_row; }

    friend std::ostream& operator<<(std::ostream& out, RowWithColumnNames const& row);

private:
    Row m_row;
    Table const& m_table;
};

// Column
class Column {
public:
    explicit Column(std::string name)
        : m_name(name) { }

    std::string name() const { return m_name; }

    std::string to_string() const { return "Column(" + m_name + ")"; }

private:
    std::string m_name;
};

// SelectResult
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

// Table
class Table : public NonCopyable {
public:
    std::optional<std::pair<Column, size_t>> get_column(std::string const& name) const;
    size_t size() const { return m_rows.size(); }
    std::vector<Column> const& columns() const { return m_columns; }

    DbErrorOr<void> add_column(Column);
    DbErrorOr<void> insert(RowWithColumnNames::MapType);
    SelectResult select() const;

private:
    friend SelectIterator;

    std::vector<Row> m_rows;
    std::vector<Column> m_columns;
};

// Row.cpp START
DbErrorOr<RowWithColumnNames> RowWithColumnNames::from_map(Table const& table, MapType map) {
    std::vector<Value> row;
    auto& columns = table.columns();
    row.resize(columns.size());
    for (auto& value : map) {
        auto column = table.get_column(value.first);
        if (!column)
            return DbError { "No such column in table: " + value.first };

        // TODO: Type check
        row[column->second] = std::move(value.second);
    }
    // TODO: Null check
    return RowWithColumnNames { Row { row }, table };
}

std::ostream& operator<<(std::ostream& out, RowWithColumnNames const& row) {
    size_t index = 0;
    out << "{ ";
    for (auto& value : row.m_row) {
        auto column = row.m_table.columns()[index];
        out << column.name() << ": ";
        if (value.has_value())
            out << *value;
        else
            out << "null";
        if (index != row.m_row.value_count() - 1)
            out << ", ";
        index++;
    }
    return out << " }";
}
// Row.cpp END

// SelectResult.cpp START
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
// SelectResult.cpp END

DbErrorOr<void> Table::add_column(Column column) {
    if (get_column(column.name()))
        return DbError { "Duplicate column '" + column.name() + "'" };
    m_columns.push_back(std::move(column));
    return {};
}

DbErrorOr<void> Table::insert(RowWithColumnNames::MapType map) {
    m_rows.push_back(TRY(RowWithColumnNames::from_map(*this, map)).row());
    return {};
}

std::optional<std::pair<Column, size_t>> Table::get_column(std::string const& name) const {
    size_t index = 0;
    for (auto& column : m_columns) {
        if (column.name() == name)
            return { { column, index } };
        index++;
    }
    return {};
}

SelectResult Table::select() const {
    return SelectResult { *this };
}

// Database
class Database : public NonCopyable {
public:
    void create_table(std::string name);
    DbErrorOr<Table*> table(std::string name);

private:
    std::unordered_map<std::string, Table> m_tables;
};

void Database::create_table(std::string name) {
    m_tables.emplace(name, Table());
}

DbErrorOr<Table*> Database::table(std::string name) {
    auto it = m_tables.find(name);
    if (it == m_tables.end())
        return DbError { "Nonexistent table: " + name };
    return &it->second;
}

DbErrorOr<void> run_query(Database& db) {
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Column("id")));
    TRY(table->add_column(Column("number")));

    TRY(table->insert({ { "id", 0 }, { "number", 69 } }));
    TRY(table->insert({ { "id", 1 }, { "number", 2137 } }));

    auto rows = table->select();

    std::cout << "SELECT * FROM test" << std::endl;
    for (auto row : rows)
        std::cout << row << std::endl;

    return {};
}

// main
int main() {
    Database db;
    auto maybe_error = run_query(db);
    if (maybe_error.is_error()) {
        auto error = maybe_error.release_error();
        std::cout << "Query failed: " << error.message() << std::endl;
    }
}
