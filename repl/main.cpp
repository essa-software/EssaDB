#include <db/sql/Lexer.hpp>
#include <db/sql/Parser.hpp>

#include <iostream>
#include <sstream>

int main() {
    std::string in_text = "SELECT * FROM test";
    std::istringstream in { in_text };
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    for (auto const& token : tokens) {
        std::cout << (int)token.type << ": " << token.value << std::endl;
    }

    Db::Sql::Parser parser { std::move(tokens) };
    auto result = parser.parse_select();
    if (result.is_error()) {
        std::cout << "Parse failed: " << result.release_error().message() << std::endl;
        return 1;
    }

    return 0;
}
