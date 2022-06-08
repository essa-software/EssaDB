#include <db/sql/Lexer.hpp>

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
    return 0;
}
