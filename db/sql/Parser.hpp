#pragma once

#include <db/core/AST.hpp>

#include "Lexer.hpp"

namespace Db::Sql {

class Parser {
public:
    Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) { }

    Core::DbErrorOr<Core::AST::Select> parse_select();

private:
    Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> parse_expression();

    std::vector<Token> m_tokens;
    size_t m_offset = 0;
};

}