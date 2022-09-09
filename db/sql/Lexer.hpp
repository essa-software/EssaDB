#pragma once

#include <string>
#include <vector>

namespace Db::Sql {

struct Token {
    enum class Type {
        KeywordAdd,
        KeywordAll,
        KeywordAlter,
        KeywordAnd,
        KeywordAs,
        KeywordBetween,
        KeywordBy,
        KeywordCase,
        KeywordCheck,
        KeywordColumn,
        KeywordConstraint,
        KeywordCreate,
        KeywordDefault,
        KeywordDelete,
        KeywordDistinct,
        KeywordDrop,
        KeywordElse,
        KeywordEnd,
        KeywordFrom,
        KeywordFull,
        KeywordGroup,
        KeywordHaving,
        KeywordImport,
        KeywordIn,
        KeywordInner,
        KeywordInsert,
        KeywordInto,
        KeywordIs,
        KeywordJoin,
        KeywordKey,
        KeywordLeft,
        KeywordLike,
        KeywordMatch,
        KeywordNot,
        KeywordNull,
        KeywordOn,
        KeywordOr,
        KeywordOrder,
        KeywordOuter,
        KeywordOver,
        KeywordPartition,
        KeywordPrimary,
        KeywordRight,
        KeywordSelect,
        KeywordSet,
        KeywordTable,
        KeywordThen,
        KeywordTop,
        KeywordTruncate,
        KeywordUnion,
        KeywordUnique,
        KeywordUpdate,
        KeywordValues,
        KeywordWhen,
        KeywordWhere,

        Asterisk,
        Bool,
        Comma,
        Date,
        Exclamation,
        Float,
        Identifier,
        Int,
        OrderByParam,
        ParenClose,
        ParenOpen,
        Period,
        Semicolon,
        String,

        OpEqual,    // =
        OpGreater,  // >
        OpLess,     // <
        OpNotEqual, // !=

        OpAdd, // +
        OpSub, // -
        OpMul, // *
        OpDiv, // /

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
