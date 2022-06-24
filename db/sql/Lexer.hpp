#pragma once

#include <string>
#include <vector>

namespace Db::Sql {

struct Token {
    enum class Type {
        KeywordAdd,
        KeywordAlias,
        KeywordAll,
        KeywordAlter,
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
        KeywordGroup,
        KeywordHaving,
        KeywordImport,
        KeywordIn,
        KeywordInsert,
        KeywordInto,
        KeywordIs,
        KeywordKey,
        KeywordOrder,
        KeywordOver,
        KeywordPartition,
        KeywordPrimary,
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
        Null,
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

        OpAnd,  // &&
        OpOr,   // ||
        OpNot,  // !
        OpLike, // LIKE

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
