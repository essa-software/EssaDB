#include <db/core/Database.hpp>
#include <db/sql/Lexer.hpp>
#include <db/sql/Parser.hpp>
#include <db/sql/SQL.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

void run_query(Db::Core::Database& db, std::string const& query) {
    std::istringstream in { query };
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    // for (auto const& token : tokens) {
    //     std::cout << (int)token.type << ": " << token.value << std::endl;
    // }

    Db::Sql::Parser parser { tokens };
    auto statement = parser.parse_statement();
    if (statement.is_error()) {
        auto error = statement.release_error();
        Db::Sql::display_error(error, tokens[error.token()].start, query);
        return;
    }
    auto result = statement.release_value()->execute(db);
    if (result.is_error()) {
        auto error = result.release_error();
        Db::Sql::display_error(error, tokens[error.token()].start, query);
        return;
    }
    result.release_value().repl_dump(std::cerr, Db::Core::ResultSet::FancyDump::Yes);
}

int main() {
    Db::Core::Database db;
    db.create_table("test", nullptr);
    while (true) {
        std::cout << "> \e[32m";
        std::string query;
        if (!std::getline(std::cin, query))
            return 1;
        std::cout << "\e[m";

        run_query(db, query);
    }
    return 0;
}
