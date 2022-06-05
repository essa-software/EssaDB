#include <core/Database.hpp>

#include <iomanip>
#include <iostream>

Db::Core::DbErrorOr<void> run_query(Db::Core::Database& db) {
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Db::Core::Column("id")));
    TRY(table->add_column(Db::Core::Column("number")));

    TRY(table->insert({ { "id", 0 }, { "number", 69 } }));
    TRY(table->insert({ { "id", 1 }, { "number", 2137 } }));

    std::cout << "SELECT * FROM test" << std::endl;
    auto rows = table->select();

    std::vector<int> widths;
    for (auto& column : rows.column_names()) {
        std::cout << "| " << column << " ";
        widths.push_back(column.size());
    }
    std::cout << "|" << std::endl;

    for (auto row : rows) {
        size_t index = 0;
        for (auto value : row) {
            std::cout << "| " << std::setw(widths[index]);
            if (value.has_value())
                std::cout << value.value();
            else
                std::cout << "null";
            std::cout << " ";
            index++;
        }
        std::cout << "|" << std::endl;
    }

    return {};
}

// main
int main() {
    Db::Core::Database db;
    auto maybe_error = run_query(db);
    if (maybe_error.is_error()) {
        auto error = maybe_error.release_error();
        std::cout << "Query failed: " << error.message() << std::endl;
    }
}
