#pragma once

#include <string>
#include <vector>

namespace Db::Sql {

struct Token {
    enum class Type {
        KeywordSelect,
        KeywordFrom,
        KeywordTop,
        KeywordCreate,
        KeywordTable,
        KeywordAlias,
        KeywordOrder,
        KeywordBy,
        KeywordWhere,
        KeywordBetween,
        KeywordInsert,
        KeywordInto,
        KeywordValues,
        KeywordDrop,
        KeywordTruncate,
        KeywordAlter,
        KeywordAdd,
        KeywordColumn,
        KeywordDistinct,
        KeywordDelete,

        OrderByParam,
        Identifier,
        Number,
        String,
        Null,
        Asterisk,
        Comma,
        ParenOpen,
        ParenClose,
        Semicolon,
        Exclamation,

        OpEqual,    // =
        OpNotEqual, // !=
        OpLess,     // <
        OpGreater,  // >

        OpAnd,
        OpOr,
        OpNot,
        OpLike,

        Eof,
        Garbage
    };

    Type type {};
    std::string value {};
    ssize_t start {};
};

class Lexer {
public:
    Lexer(std::istream& in)
        : m_in(in) { }

    std::vector<Token> lex();

private:
    std::istream& m_in;
};

}
