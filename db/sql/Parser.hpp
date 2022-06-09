#pragma once

#include <db/core/AST.hpp>

#include "Lexer.hpp"

namespace Db::Sql {

class Parser {
public:
    Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) { }

    Core::DbErrorOr<std::unique_ptr<Core::AST::Statement>> parse_statement();

private:
    Core::DbErrorOr<std::unique_ptr<Core::AST::Select>> parse_select();
    Core::DbErrorOr<std::unique_ptr<Core::AST::CreateTable>> parse_create_table();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> parse_expression();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Function>> parse_function(std::string name);

    std::vector<Token> m_tokens;
    size_t m_offset = 0;
};

}
