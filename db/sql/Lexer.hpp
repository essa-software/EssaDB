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
        KeywordEngine,
        KeywordFrom,
        KeywordForeign,
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
        KeywordReferences,
        KeywordSelect,
        KeywordSet,
        KeywordShow,
        KeywordTable,
        KeywordTables,
        KeywordThen,
        KeywordTop,
        KeywordTruncate,
        KeywordUnion,
        KeywordUnique,
        KeywordUpdate,
        KeywordValues,
        KeywordWhen,
        KeywordWhere,

        __KeywordCount,

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
    ssize_t end {};

    bool is_keyword() const {
        return static_cast<int>(type) < static_cast<int>(Type::__KeywordCount);
    }
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
