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
    db.create_table("test");
    auto table = TRY(db.table("test"));

    TRY(table->add_column(Column("id", Value::Type::Int)));
    TRY(table->add_column(Column("number", Value::Type::Int)));
    TRY(table->add_column(Column("string", Value::Type::Varchar)));

    TRY(table->insert({ { "id", Value::create_int(0) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test") } }));
    TRY(table->insert({ { "id", Value::create_int(1) }, { "number", Value::create_int(2137) } }));
    TRY(table->insert({ { "id", Value::create_int(2) }, { "number", Value::null() } }));
    TRY(table->insert({ { "id", Value::create_int(3) }, { "number", Value::create_int(420) } }));
    TRY(table->insert({ { "id", Value::create_int(4) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test3") } }));
    TRY(table->insert({ { "id", Value::create_int(5) }, { "number", Value::create_int(69) }, { "string", Value::create_varchar("test2") } }));
    return db;
}

DbErrorOr<void> select_simple() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT * FROM test")).to_select_result());
    TRY(expect(result.column_names() == std::vector<std::string> { "id", "number", "string" }, "columns have proper names"));
    TRY(expect(result.rows().size() == 6, "all rows were returned"));
    TRY(expect(result.rows()[0].value_count() == 3, "rows have proper column count"));
    return {};
}

DbErrorOr<void> select_columns() {
    auto db = TRY(setup_db());
    // TODO: Returns columns in given order, not in table order
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT number, string FROM test")).to_select_result());
    TRY(expect(result.column_names() == std::vector<std::string> { "number", "string" }, "columns have proper names"));
    TRY(expect(result.rows().size() == 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_where() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                              { std::vector<std::string> { "id", "number" } },
                              "test",
                              AST::Filter { .column = "id", .operation = AST::Filter::Operation::Equal, .rhs = Value::create_int(2) })
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    TRY(expect(result.rows()[0].value(1).type() == Value::Type::Null, "number is null as in table"));
    return {};
}

DbErrorOr<void> select_where_operator() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                              { { "id" } },
                              "test",
                              AST::Filter { .column = "id", .operation = AST::Filter::Operation::LessEqual, .rhs = Value::create_int(2) })
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_order_by() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number", "string" } },
                              "test",
                              {},
                              AST::OrderBy { .columns = {
                                    AST::OrderBy::OrderBySet{.name = "string", .order = AST::OrderBy::Order::Ascending},
                                    AST::OrderBy::OrderBySet{.name = "number", .order = AST::OrderBy::Order::Descending},
                                } } )
                              .execute(db))
                          .to_select_result());
    TRY(expect(TRY(result.rows()[1].value(1).to_string()) < TRY(result.rows()[5].value(1).to_string()), "values are sorted"));
    return {};
}

DbErrorOr<void> select_order_by_desc() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number", "string" } },
                              "test",
                              {},
                              AST::OrderBy { .columns = {AST::OrderBy::OrderBySet{.name = "number", .order = AST::OrderBy::Order::Descending} } } )
                              .execute(db))                         
                            .to_select_result());
    TRY(expect(TRY(result.rows()[1].value(1).to_string()) > TRY(result.rows()[5].value(1).to_string()), "values are sorted"));
    return {};
}

DbErrorOr<void> select_top_number() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT TOP 2 * FROM test")).to_select_result());
    TRY(expect(result.rows().size() == 2, "select result is truncated to specified value"));
    return {};
}

DbErrorOr<void> select_top_perc() {
    auto db = TRY(setup_db());
    auto result = TRY(TRY(Db::Sql::run_query(db, "SELECT TOP 75 PERC * FROM test")).to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 4, "select result is truncated to specified value"));
    return {};
}

std::map<std::string, TestFunc*> get_tests() {
    return {
        { "select_simple", select_simple },
        { "select_columns", select_columns },
        { "select_where", select_where },
        { "select_where_operator", select_where_operator },
        { "select_order_by", select_order_by },
        { "select_order_by_desc", select_order_by_desc },
        { "select_top_number", select_top_number },
        { "select_top_perc", select_top_perc },
    };
}
