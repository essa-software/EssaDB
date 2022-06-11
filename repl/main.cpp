#include <db/core/Database.hpp>
#include <db/sql/Lexer.hpp>
#include <db/sql/Parser.hpp>
#include <db/sql/SQL.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

void display_error(Db::Core::DbError const& error, ssize_t error_start, std::string const& query) {
    std::cout << "\e[1;31mError: \e[m" << error.message() << std::endl;

    std::istringstream iss { query };
    ssize_t line_number = 1;
    ssize_t last_index = 0;
    while (true) {
        std::string str;
        if (!std::getline(iss, str))
            break;
        std::cout << std::setw(4) << line_number << " | ";
        for (ssize_t s = 0; static_cast<size_t>(s) < str.size(); s++) {
            if (s + last_index == error_start)
                std::cout << "\e[31m" << str[s] << "\e[m";
            else
                std::cout << str[s];
        }
        std::cout << std::endl;
        if (last_index <= error_start && last_index + str.size() >= static_cast<size_t>(error_start)) {
            std::cout << "       ";
            for (ssize_t s = 0; s < error_start - last_index; s++)
                std::cout << " ";
            std::cout << "^ \e[31m" << error.message() << "\e[m" << std::endl;
        }
        last_index = iss.tellg();
    }
}

void run_query(Db::Core::Database& db, std::string const& query) {
    std::istringstream in { query };
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    // for (auto const& token : tokens) {
    //     std::cout << (int)token.type << ": " << token.value << std::endl;
    // }

    Db::Sql::Parser parser { tokens, db};
    auto statement = parser.parse_statement();
    if (statement.is_error()) {
        auto error = statement.release_error();
        display_error(error, tokens[error.token()].start, query);
        return;
    }
    auto result = statement.release_value()->execute(db);
    if (result.is_error()) {
        auto error = result.release_error();
        display_error(error, tokens[error.token()].start, query);
        return;
    }
    result.release_value().repl_dump(std::cerr);
}

int main() {
    Db::Core::Database db;
    db.create_table("test");
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
