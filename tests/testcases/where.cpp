#include <tests/setup.hpp>

#include <db/core/AST.hpp>
#include <db/core/Database.hpp>
#include <db/core/SelectResult.hpp>
#include <db/sql/SQL.hpp>

#include <iostream>
#include <string>
#include <utility>

using namespace Db::Core;

DbErrorOr<Database> setup_db() {
    Database db;
    TRY(Db::Sql::run_query(db, "CREATE TABLE test (id INT, number INT, string VARCHAR)"));
    auto table = TRY(db.table("test"));

    TRY(table->insert({ { "id", Value::create_int(0) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test") } }));
    TRY(table->insert({ { "id", Value::create_int(1) }, { "number", Value::create_int(2137) } }));
    TRY(table->insert({ { "id", Value::create_int(2) }, { "number", Value::null() } }));
    TRY(table->insert({ { "id", Value::create_int(3) }, { "number", Value::create_int(420) } }));
    TRY(table->insert({ { "id", Value::create_int(4) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test3") } }));
    TRY(table->insert({ { "id", Value::create_int(5) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test2") } }));
    return db;
}

DbErrorOr<void> select_where() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                              std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}},
                              "test",
                              AST::Filter { .filter_rules = { AST::Filter::FilterSet{.column = "id", .operation = AST::Filter::Operation::Equal, .args = {Value::create_int(2)} }}})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    TRY(expect(result.rows()[0].value(1).type() == Value::Type::Null, "number is null as in table"));
    return {};
}

DbErrorOr<void> select_where_multiple_rules() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "id", .operation = AST::Filter::Operation::LessEqual, .args = {Value::create_int(2)}, .logic = AST::Filter::LogicOperator::AND},
                                AST::Filter::FilterSet{.column = "id", .operation = AST::Filter::Operation::Equal, .args = {Value::create_int(2)}},
                            }})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    TRY(expect(result.rows()[0].value(1).type() == Value::Type::Null, "number is null as in table"));
    return {};
}

DbErrorOr<void> select_where_between() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}},
                              "test",
                              AST::Filter { .filter_rules = { AST::Filter::FilterSet{.column = "id", .operation = AST::Filter::Operation::Between, .args = {Value::create_int(2), Value::create_int(4)} }}})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_where_in_statement() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}},
                              "test",
                              AST::Filter { .filter_rules = { AST::Filter::FilterSet{.column = "id", .operation = AST::Filter::Operation::In, .args = {Value::create_int(1), Value::create_int(5)} }}})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_but_more_complex() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}, {.column = "string", .alias = "STRING"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "number", .operation = AST::Filter::Operation::Equal, .args = {Value::create_int(69)}, .logic = AST::Filter::LogicOperator::AND},
                                AST::Filter::FilterSet{.column = "string", .operation = AST::Filter::Operation::Equal, .args = {Value::create_varchar("test2")}, .logic = AST::Filter::LogicOperator::AND},
                            }})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    TRY(expect(result.rows()[0].value(1).to_int().value() == 69 && result.rows()[0].value(2).to_string().value() == "test2", "returned correct values"));
    return {};
}

DbErrorOr<void> select_where_like_without_asterisks() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}, {.column = "string", .alias = "STRING"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "string", .operation = AST::Filter::Operation::Like, .args = {Value::create_varchar("test?")}, .logic = AST::Filter::LogicOperator::AND},
                            }})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_prefix_asterisk() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}, {.column = "string", .alias = "STRING"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "string", .operation = AST::Filter::Operation::Like, .args = {Value::create_varchar("*st?")}, .logic = AST::Filter::LogicOperator::AND},
                            }})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 2, "2 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_suffix_asterisk() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id"}, {.column = "number", .alias = "NUMBER_ALIAS"}, {.column = "string", .alias = "STRING"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "string", .operation = AST::Filter::Operation::Like, .args = {Value::create_varchar("te*")}, .logic = AST::Filter::LogicOperator::AND},
                            }})
                              .execute(db))
                          .to_select_result());
    result.dump(std::cout);
    TRY(expect(result.rows().size() == 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_suffix_asterisk_and_in_statement() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}, {.column = "string", .alias = "STRING"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "string", .operation = AST::Filter::Operation::Like, .args = {Value::create_varchar("te*")}, .logic = AST::Filter::LogicOperator::AND},
                                AST::Filter::FilterSet{.column = "id", .operation = AST::Filter::Operation::In, .args = {Value::create_int(1), Value::create_int(5)}, .logic = AST::Filter::LogicOperator::AND},
                            }})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    return {};
}

DbErrorOr<void> select_where_like_with_two_asterisks() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                            std::vector<AST::SelectColumns::Column> {{.column = "id", .alias = "ID"}, {.column = "number", .alias = "NUMBER"}, {.column = "string", .alias = "STRING"}},
                              "test",
                              AST::Filter { .filter_rules = { 
                                AST::Filter::FilterSet{.column = "string", .operation = AST::Filter::Operation::Like, .args = {Value::create_varchar("*st*")}, .logic = AST::Filter::LogicOperator::AND},
                            }})
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 2, "2 rows returned"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "select_where", select_where },
        { "select_where_multiple_rules", select_where_multiple_rules },
        { "select_where_between", select_where_between },
        { "select_where_in_statement", select_where_in_statement },
        { "select_where_but_more_complex", select_where_but_more_complex },
        { "select_where_like_without_asterisks", select_where_like_without_asterisks },
        { "select_where_like_with_prefix_asterisk", select_where_like_with_prefix_asterisk },
        { "select_where_like_with_suffix_asterisk", select_where_like_with_suffix_asterisk },
        { "select_where_like_with_two_asterisks", select_where_like_with_two_asterisks },
        { "select_where_like_with_suffix_asterisk_and_in_statement", select_where_like_with_suffix_asterisk_and_in_statement },
    };
}
