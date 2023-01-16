#include "CommandLine.hpp"
#include <EssaUtil/DisplayError.hpp>
#include <EssaUtil/Stream.hpp>
#include <EssaUtil/Stream/File.hpp>
#include <db/core/Database.hpp>
#include <db/core/DatabaseEngine.hpp>
#include <db/sql/Lexer.hpp>
#include <db/sql/Parser.hpp>
#include <db/sql/SQL.hpp>

#include <fstream>
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
        Db::Sql::display_error(error, tokens[error.token()].start, tokens[error.token()].end, query);
        return;
    }
    auto result = statement.release_value()->execute(db);
    if (result.is_error()) {
        auto error = result.release_error();
        Db::Sql::display_error(error, tokens[error.token()].start, tokens[error.token()].end, query);
        return;
    }
    result.release_value().repl_dump(std::cerr, Db::Core::ResultSet::FancyDump::Yes);
}

void display_error(Db::Core::DbError const& error, ssize_t error_start, ssize_t error_end, std::string const& file_name) {
    auto stream = MUST(Util::ReadableFileStream::open(file_name));
    Util::display_error(stream,
        Util::DisplayedError {
            .message = Util::UString { error.message() },
            .start_offset = static_cast<size_t>(error_start),
            .end_offset = static_cast<size_t>(error_end),
        });
}

int run_sql_file(Db::Core::Database& db, std::string const& file_name) {
    std::ifstream in { file_name };
    if (!in.good()) {
        fmt::print("Failed to open SQL file: '{}'\n", file_name);
        return 1;
    }
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    // for (auto const& token : tokens) {
    //     std::cout << (int)token.type << ": " << token.value << std::endl;
    // }

    Db::Sql::Parser parser { tokens };
    auto statement_list = parser.parse_statement_list();

    if (statement_list.is_error()) {
        auto error = statement_list.release_error();

        auto query = Util::ReadableFileStream::read_file(file_name);
        if (query.is_error()) {
            query = Util::Buffer {};
        }
        display_error(error, tokens[error.token()].start, tokens[error.token()].end, query.release_value().decode_infallible().encode());
        return 1;
    }
    auto result = statement_list.release_value().execute(db);
    if (result.is_error()) {
        auto error = result.release_error();

        auto query = Util::ReadableFileStream::read_file(file_name);
        if (query.is_error()) {
            query = Util::Buffer {};
        }
        display_error(error, tokens[error.token()].start, tokens[error.token()].end, query.release_value().decode_infallible().encode());
        return 1;
    }
    result.release_value().repl_dump(std::cerr, Db::Core::ResultSet::FancyDump::Yes);
    return 0;
}

int main(int argc, char* argv[]) {
    Db::Core::Database db;
    db.set_default_engine(Db::Core::DatabaseEngine::EDB);

    if (argc == 1) {
        MUST(db.create_table({ "test", {} }, nullptr, Db::Core::DatabaseEngine::Memory));
        CommandLine::Line line;
        line.set_prompt("> \e[32m");

        while (true) {
            auto query = line.get();
            if (query.is_error()) {
                std::optional<int> exit_code;
                std::visit(
                    Util::Overloaded {
                        [&](CommandLine::GetLineResult result) {
                            switch (result) {
                            case CommandLine::GetLineResult::EndOfFile:
                                exit_code = 0;
                                break;
                            }
                        },
                        [&](Util::OsError const& error) {
                            fmt::print("Error while reading line: {}\n", error);
                            exit_code = 2;
                        } },
                    query.release_error_variant());
                if (exit_code) {
                    return *exit_code;
                }
            }
            std::cout << "\e[m";
            run_query(db, query.release_value().encode());
        }
    }
    else if (argc == 2) {
        return run_sql_file(db, argv[1]);
    }
    else {
        fmt::print("Usage: essadb-repl [file name]\n");
        return 1;
    }
    return 0;
}
