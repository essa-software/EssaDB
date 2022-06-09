#include <db/core/Database.hpp>
#include <db/sql/Lexer.hpp>
#include <db/sql/Parser.hpp>
#include <db/sql/SQL.hpp>

#include <iostream>
#include <sstream>

int main() {
    Db::Core::Database db;
    db.create_table("test");
    while (true) {
        std::cout << "> ";
        std::string query;
        if (!std::getline(std::cin, query))
            return 1;

        auto result = Db::Sql::run_query(db, query);
        if (result.is_error())
            std::cout << "Error: " << result.release_error().message() << std::endl;
        else
            result.release_value().repl_dump(std::cout);
    }
    return 0;
}
