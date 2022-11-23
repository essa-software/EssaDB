#include "SQL.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"

#include <EssaUtil/DisplayError.hpp>
#include <EssaUtil/Stream/MemoryStream.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Db::Sql {

Core::DbErrorOr<Core::ValueOrResultSet> run_query(Core::Database& db, std::string const& query) {
    std::istringstream in { query };
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    // for (auto const& token : tokens) {
    //     std::cout << (int)token.type << ": " << token.value << std::endl;
    // }

    Db::Sql::Parser parser { std::move(tokens) };
    auto statement = TRY(parser.parse_statement());
    auto result = TRY(statement->execute(db));
    // result.repl_dump(std::cerr);
    return result;
}

void display_error(Db::Core::DbError const& error, ssize_t error_start, ssize_t error_end, std::string const& query) {
    Util::ReadableMemoryStream stream { { reinterpret_cast<uint8_t const*>(query.c_str()), query.size() } };
    Util::display_error(stream,
        Util::DisplayedError {
            .message = Util::UString { error.message() },
            .start_offset = static_cast<size_t>(error_start),
            .end_offset = static_cast<size_t>(error_end),
        });
}

}
