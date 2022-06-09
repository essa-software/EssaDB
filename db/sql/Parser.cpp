#include "Parser.hpp"
#include "db/core/AST.hpp"
#include "db/core/Database.hpp"
#include "db/sql/Lexer.hpp"
#include <iostream>
#include <optional>

namespace Db::Sql {

Core::DbErrorOr<Core::AST::Select> Parser::parse_select() {
    auto select = m_tokens[m_offset++];
    if (select.type != Token::Type::KeywordSelect)
        return Core::DbError { "Expected 'SELECT'" };

    std::optional<Core::AST::Filter> filter;
    std::optional<Core::AST::OrderBy> order;
    std::optional<Core::AST::Top> top;

    if(m_tokens[m_offset].type == Token::Type::KeywordAfterSelect){
        if(m_tokens[m_offset++].value == "TOP"){
            try{
                unsigned value = std::stoi(m_tokens[m_offset++].value); 
                if(m_tokens[m_offset].value == "PERC"){
                    top = Core::AST::Top{.unit = Core::AST::Top::Unit::Perc, .value = value};

                    m_offset++;
                }else
                    top = Core::AST::Top{.unit = Core::AST::Top::Unit::Val, .value = value};
            }catch(...){
                return Core::DbError { "Invalid argument for TOP" };
            }
        }
    }

    std::set<std::string> columns;
    while (true) {
        auto column = m_tokens[m_offset++];
        if (column.type == Token::Type::Asterisk)
            break;

        if (column.type == Token::Type::Identifier)
            columns.insert(column.value);
        else
            return Core::DbError { "Expected column name" };

        auto comma = m_tokens[m_offset];
        if (comma.type != Token::Type::Comma)
            break;
        m_offset++;
    }

    auto from = m_tokens[m_offset++];
    if (from.type != Token::Type::KeywordFrom)
        return Core::DbError { "Expected 'FROM', got " + from.value };

    auto from_token = m_tokens[m_offset++];
    if (from_token.type != Token::Type::Identifier)
        return Core::DbError { "Expected table name after 'FROM'" };

    return Core::AST::Select { { columns }, from_token.value, std::move(filter), std::move(order), std::move(top) };
}

}
