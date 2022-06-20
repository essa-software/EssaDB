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
        KeywordIn,
        KeywordGroup,
        KeywordHaving,
        KeywordCase,
        KeywordWhen,
        KeywordThen,
        KeywordEnd,
        KeywordElse,
        KeywordIs,
        KeywordUpdate,
        KeywordSet,
        KeywordUnion,
        KeywordAll,
        KeywordImport,
        KeywordUnique,
        KeywordDefault,
        KeywordCheck,
        KeywordConstraint,

        OrderByParam,
        Identifier,
        Int,
        Float,
        String,
        Bool,
        Date,
        Null,
        Asterisk,
        Comma,
        ParenOpen,
        ParenClose,
        Semicolon,
        Exclamation,
        Period,

        OpEqual,    // =
        OpNotEqual, // !=
        OpLess,     // <
        OpGreater,  // >

        OpAdd,      // +
        OpSub,      // -
        OpMul,      // *
        OpDiv,      // /

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
