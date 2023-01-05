#pragma once

#include "Lexer.hpp"

#include <db/core/Column.hpp>
#include <db/core/Database.hpp>
#include <db/sql/SQLError.hpp>
#include <db/sql/Select.hpp>
#include <db/sql/ast/Expression.hpp>
#include <db/sql/ast/Function.hpp>
#include <db/sql/ast/Select.hpp>
#include <db/sql/ast/Statement.hpp>
#include <db/sql/ast/TableExpression.hpp>
#include <memory>

namespace Db::Sql {

class Parser {
public:
    // NOTE: This stores a reference.
    explicit Parser(std::vector<Token> const& tokens)
        : m_tokens(std::move(tokens)) { }

    SQLErrorOr<std::unique_ptr<AST::Statement>> parse_statement();
    SQLErrorOr<AST::StatementList> parse_statement_list();
    bool static compare_case_insensitive(std::string const& lhs, std::string const& rhs);

private:
    SQLErrorOr<AST::Select> parse_select();
    SQLErrorOr<std::unique_ptr<AST::CreateTable>> parse_create_table();
    SQLErrorOr<std::unique_ptr<AST::DropTable>> parse_drop_table();
    SQLErrorOr<std::unique_ptr<AST::TruncateTable>> parse_truncate_table();
    SQLErrorOr<std::unique_ptr<AST::AlterTable>> parse_alter_table();
    SQLErrorOr<std::unique_ptr<AST::InsertInto>> parse_insert_into();
    SQLErrorOr<std::unique_ptr<AST::DeleteFrom>> parse_delete_from();
    SQLErrorOr<std::unique_ptr<AST::Update>> parse_update();
    SQLErrorOr<std::unique_ptr<AST::Import>> parse_import();
    SQLErrorOr<std::unique_ptr<AST::Expression>> parse_expression(int min_precedence = 0);
    SQLErrorOr<std::unique_ptr<AST::Expression>> parse_expression_or_index(Sql::AST::SelectColumns const&);
    SQLErrorOr<std::optional<Core::DatabaseEngine>> parse_engine_specification();

    struct BetweenRange {
        std::unique_ptr<Sql::AST::Expression> min;
        std::unique_ptr<Sql::AST::Expression> max;

        BetweenRange(std::unique_ptr<Sql::AST::Expression> min, std::unique_ptr<Sql::AST::Expression> max)
            : min(std::move(min))
            , max(std::move(max)) { }
    };

    struct InArgs {
        std::vector<std::unique_ptr<Sql::AST::Expression>> args;

        InArgs(std::vector<std::unique_ptr<Sql::AST::Expression>> arg_list)
            : args(std::move(arg_list)) { }
    };

    struct IsArgs {
        Sql::AST::IsExpression::What what {};

        explicit IsArgs(Sql::AST::IsExpression::What what)
            : what(what) { }
    };

    SQLErrorOr<Parser::BetweenRange> parse_between_range();                                                                        // (BETWEEN) x AND y
    SQLErrorOr<std::unique_ptr<AST::Expression>> parse_operand(std::unique_ptr<Sql::AST::Expression> lhs, int min_precedence = 0); // parses operator + rhs
    SQLErrorOr<std::unique_ptr<AST::Expression>> parse_function(std::string name);
    SQLErrorOr<Parser::InArgs> parse_in();
    SQLErrorOr<Parser::IsArgs> parse_is();
    SQLErrorOr<AST::TableStatement::ExistanceCondition> parse_table_existence();
    SQLErrorOr<std::unique_ptr<AST::Identifier>> parse_identifier();
    SQLErrorOr<std::unique_ptr<AST::Literal>> parse_literal();
    SQLErrorOr<AST::ParsedColumn> parse_column();
    SQLErrorOr<std::unique_ptr<AST::TableExpression>> parse_table_expression();
    SQLErrorOr<std::unique_ptr<AST::TableIdentifier>> parse_table_identifier();
    SQLErrorOr<std::unique_ptr<AST::TableExpression>> parse_join_expression(std::unique_ptr<AST::TableExpression> lhs);

    std::vector<Token> const& m_tokens;

    static SQLError expected(std::string what, Token got, size_t offset);

    size_t m_offset = 0;
};

}
