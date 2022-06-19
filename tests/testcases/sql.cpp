#include <tests/setup.hpp>

#include <db/sql/Lexer.hpp>
#include <db/sql/Parser.hpp>
#include <db/sql/SQL.hpp>

#include <filesystem>
#include <fstream>

using namespace Db::Core;

std::string trim(std::string str) {
    auto first_space = str.find_first_not_of(' ');
    if (first_space != std::string::npos)
        str = str.substr(first_space);
    auto last_space = str.find_last_not_of(' ');
    if (first_space < str.size() - 1)
        str = str.substr(0, last_space + 1);
    return str;
}

struct SQLStatement {
    bool display = false;
    bool skip = false;
    std::string statement;
    std::optional<std::string> expected_output;
    std::optional<std::string> expected_error;
};

Db::Core::DbErrorOr<Value> run_query(Db::Core::Database& db, SQLStatement const& sql_statement) {
    if (sql_statement.display)
        std::cout << "> \e[32m" << sql_statement.statement << "\e[m" << std::endl;

    std::istringstream in { sql_statement.statement };
    Db::Sql::Lexer lexer { in };
    auto tokens = lexer.lex();
    // for (auto const& token : tokens) {
    //     std::cout << (int)token.type << ": " << token.value << std::endl;
    // }

    Db::Sql::Parser parser { tokens };
    auto statement = parser.parse_statement();
    if (statement.is_error()) {
        auto error = statement.release_error();
        if (sql_statement.display)
            Db::Sql::display_error(error, tokens[error.token()].start, sql_statement.statement);
        return error;
    }
    auto result = statement.release_value()->execute(db);
    if (result.is_error()) {
        auto error = result.release_error();
        if (sql_statement.display)
            Db::Sql::display_error(error, tokens[error.token()].start, sql_statement.statement);
        return error;
    }
    if (sql_statement.display)
        result.value().repl_dump(std::cerr);

    return result.release_value();
}

bool display_error_if_error(Db::Core::DbErrorOr<Db::Core::Value>&& query_result, SQLStatement const& statement) {
    if (query_result.is_error()) {
        auto error = query_result.error();
        if (statement.expected_error) {
            if (statement.display)
                std::cout << "Expecting error: \e[31m" << *statement.expected_error << "\e[m" << std::endl;
            if (error.message() != *statement.expected_error) {
                std::cout << " [FAIL] Incorrect error returned: '" << error.message() << "'" << std::endl;
            }
        }
        else {
            std::cout << " [FAIL] Expected result, but got error: " << error.message() << std::endl;
            return false;
        }
    }
    else {
        if (statement.expected_error) {
            std::cout << " [FAIL] Expected error, but got result:" << std::endl;
            query_result.release_value().repl_dump(std::cout);
            return false;
        }
        if (statement.expected_output) {
            std::ostringstream output;
            query_result.value().repl_dump(output);
            if (output.str() != *statement.expected_output) {
                std::cout << " [FAIL] Expected result: " << std::endl;
                std::cout << *statement.expected_output << std::endl;
                std::cout << " [FAIL] but got result: " << std::endl;
                std::cout << output.str() << std::endl;
                return false;
            }
        }
    }
    return true;
}

std::map<std::string, TestFunc> get_tests() {
    std::map<std::string, TestFunc> tests;

    constexpr auto TestPath = "../tests/sql";

    std::filesystem::recursive_directory_iterator directory { TestPath };

    for (auto file_it : directory) {
        auto test_func = [file_it]() -> Db::Core::DbErrorOr<void> {
            std::ifstream file { file_it.path() };

            std::vector<SQLStatement> statements;
            SQLStatement* current_statement = nullptr;

            enum class State {
                Sql,
                Output,
                Error,
            };

            State state = State::Sql;
            while (true) {
                std::string line;

                if (!std::getline(file, line))
                    break;
                if (line.empty())
                    continue;

                if (current_statement == nullptr) {
                    statements.push_back({});
                    current_statement = &statements.back();
                }

                if (line.starts_with("--")) {
                    if (state == State::Sql) {
                        auto type = line.substr(2);
                        type = trim(type);

                        if (type == "display") {
                            current_statement->display = true;
                            continue;
                        }
                        else if (type.starts_with("skip")) {
                            current_statement->skip = true;
                            continue;
                        }

                        // std::cout << "TYPE = '" << type << "'" << std::endl;

                        if (type.starts_with("output:")) {
                            state = State::Output;
                        }
                        else if (type.starts_with("error:")) {
                            state = State::Error;
                        }
                        else {
                            // Ignore normal comments
                            continue;
                        }

                        auto colon = type.find_first_of(":");
                        auto inline_data = trim(type.substr(colon + 1));
                        if (!inline_data.empty()) {
                            if (state == State::Output)
                                current_statement->expected_output = inline_data;
                            else if (state == State::Error)
                                current_statement->expected_error = inline_data;

                            state = State::Sql;
                        }
                        continue;
                    }
                }
                else {
                    state = State::Sql;
                }

                if (state == State::Sql) {
                    current_statement->statement = line;
                    current_statement = nullptr;
                }
                else {
                    auto part = trim(line.substr(3));
                    if (state == State::Output) {
                        if (!current_statement->expected_output)
                            current_statement->expected_output = "";
                        *current_statement->expected_output += part + "\n";
                    }
                    else if (state == State::Error) {
                        if (!current_statement->expected_error)
                            current_statement->expected_error = "";
                        *current_statement->expected_error += part + "\n";
                    }
                }
            }

            Db::Core::Database db;

            bool success = true;
            for (auto const& statement : statements) {
                if (statement.skip) {
                    std::cout << " [*] Skipped: " << statement.statement << std::endl;
                    continue;
                }
                success &= display_error_if_error(run_query(db, statement), statement);
            }
            if (!success)
                return Db::Core::DbError { "SQL failed", 0 };
            return {};
        };

        if (file_it.path().extension().string().ends_with(".sql")) {
            tests.insert({ file_it.path().lexically_relative(TestPath), test_func });
        }
    }
    return tests;
}
