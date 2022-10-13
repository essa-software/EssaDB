#pragma once

#include "Lexer.hpp"

#include <db/core/Column.hpp>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>
#include <db/core/Function.hpp>
#include <db/core/Select.hpp>
#include <db/core/ast/Expression.hpp>
#include <db/core/ast/Select.hpp>
#include <db/core/ast/Statement.hpp>
#include <db/core/ast/TableExpression.hpp>
#include <memory>

namespace Db::Sql {

class Parser {
public:
    // NOTE: This stores a reference.
    explicit Parser(std::vector<Token> const& tokens)
        : m_tokens(std::move(tokens)) { }

    Core::DbErrorOr<std::unique_ptr<Core::AST::Statement>> parse_statement();
    bool static compare_case_insensitive(std::string const& lhs, std::string const& rhs);

private:
    Core::DbErrorOr<Core::AST::Select> parse_select();
    Core::DbErrorOr<std::unique_ptr<Core::AST::CreateTable>> parse_create_table();
    Core::DbErrorOr<std::unique_ptr<Core::AST::DropTable>> parse_drop_table();
    Core::DbErrorOr<std::unique_ptr<Core::AST::TruncateTable>> parse_truncate_table();
    Core::DbErrorOr<std::unique_ptr<Core::AST::AlterTable>> parse_alter_table();
    Core::DbErrorOr<std::unique_ptr<Core::AST::InsertInto>> parse_insert_into();
    Core::DbErrorOr<std::unique_ptr<Core::AST::DeleteFrom>> parse_delete_from();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Update>> parse_update();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Import>> parse_import();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> parse_expression(int min_precedence = 0);
    Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> parse_expression_or_index(Core::AST::SelectColumns const&);

    struct BetweenRange {
        std::unique_ptr<Core::AST::Expression> min;
        std::unique_ptr<Core::AST::Expression> max;

        BetweenRange(std::unique_ptr<Core::AST::Expression> min, std::unique_ptr<Core::AST::Expression> max)
            : min(std::move(min))
            , max(std::move(max)) { }
    };

    struct InArgs {
        std::vector<std::unique_ptr<Core::AST::Expression>> args;

        InArgs(std::vector<std::unique_ptr<Core::AST::Expression>> arg_list)
            : args(std::move(arg_list)) { }
    };

    struct IsArgs {
        Core::AST::IsExpression::What what {};

        explicit IsArgs(Core::AST::IsExpression::What what)
            : what(what) { }
    };

    Core::DbErrorOr<Parser::BetweenRange> parse_between_range();                                                                               // (BETWEEN) x AND y
    Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> parse_operand(std::unique_ptr<Core::AST::Expression> lhs, int min_precedence = 0); // parses operator + rhs
    Core::DbErrorOr<std::unique_ptr<Core::AST::Expression>> parse_function(std::string name);
    Core::DbErrorOr<Parser::InArgs> parse_in();
    Core::DbErrorOr<Parser::IsArgs> parse_is();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Identifier>> parse_identifier();
    Core::DbErrorOr<std::unique_ptr<Core::AST::Literal>> parse_literal();
    Core::DbErrorOr<Core::Column> parse_column();
    Core::DbErrorOr<std::unique_ptr<Core::AST::TableExpression>> parse_table_expression();
    Core::DbErrorOr<std::unique_ptr<Core::AST::TableIdentifier>> parse_table_identifier();
    Core::DbErrorOr<std::unique_ptr<Core::AST::TableExpression>> parse_join_expression(std::unique_ptr<Core::AST::TableExpression> lhs);

    std::vector<Token> const& m_tokens;

    static Core::DbError expected(std::string what, Token got, size_t offset);

    size_t m_offset = 0;
};

}
