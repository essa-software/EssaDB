#include "SQL.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"

#include <iostream>
#include <sstream>

namespace Db::Sql {

Core::DbErrorOr<Core::Value> run_query(Core::Database& db, std::string const& query) {
    std::istringstream in { query };
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    // for (auto const& token : tokens) {
    //     std::cout << (int)token.type << ": " << token.value << std::endl;
    // }

    Db::Sql::Parser parser { std::move(tokens)};
    auto statement = TRY(parser.parse_statement());
    auto result = TRY(statement->execute(db));
    //result.repl_dump(std::cerr);
    return result;
}

}
