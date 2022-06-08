#include "Parser.hpp"
#include "db/core/AST.hpp"

namespace Db::Sql {

Core::DbErrorOr<Core::AST::Select> Parser::parse_select() {
    auto select = m_tokens[m_offset];
    if (select.type != Token::Type::KeywordSelect)
        return Core::DbError { "Expected 'SELECT'" };
    m_offset++;

    Core::AST::SelectColumns columns;
    auto column = m_tokens[m_offset];
    if (column.type == Token::Type::Asterisk)
        columns = {};
    else if (column.type == Token::Type::Identifier)
        columns = { { column.value } };
    else
        return Core::DbError { "Expected column name" };
    m_offset++;

    auto from = m_tokens[m_offset];
    if (from.type != Token::Type::KeywordFrom)
        return Core::DbError { "Expected 'FROM'" };
    m_offset++;

    auto from_token = m_tokens[m_offset];
    if (from_token.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name after 'FROM'" };
    m_offset++;

    return Core::AST::Select { columns, from_token.value, {}, {}, {} };
}

}
