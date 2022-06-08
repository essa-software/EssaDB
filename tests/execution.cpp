#include "setup.hpp"

#include <db/core/AST.hpp>
#include <db/core/Database.hpp>
#include <db/core/SelectResult.hpp>

#include <iostream>
#include <string>

using namespace Db::Core;

DbErrorOr<void> select_simple(Database& db) {
    auto result = TRY(TRY(AST::Select({}, "test", {}).execute(db)).to_select_result());
    TRY(expect(result.column_names() == std::vector<std::string> { "id", "number", "string" }, "columns have proper names"));
    TRY(expect(result.rows().size() == 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_columns(Database& db) {
    // TODO: Returns columns in given order, not in table order
    auto result = TRY(TRY(AST::Select({ { "number", "string" } }, "test", {}).execute(db)).to_select_result());
    TRY(expect(result.column_names() == std::vector<std::string> { "number", "string" }, "columns have proper names"));
    TRY(expect(result.rows().size() == 6, "all rows were returned"));
    return {};
}

DbErrorOr<void> select_where(Database& db) {
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number" } },
                              "test",
                              AST::Filter { .column = "id", .operation = AST::Filter::Operation::Equal, .rhs = Value::create_int(2) })
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 1, "1 row returned"));
    TRY(expect(result.rows()[0].value(1).type() == Value::Type::Null, "number is null as in table"));
    return {};
}

DbErrorOr<void> select_where_operator(Database& db) {
    auto result = TRY(TRY(AST::Select(
                              { { "id" } },
                              "test",
                              AST::Filter { .column = "id", .operation = AST::Filter::Operation::LessEqual, .rhs = Value::create_int(2) })
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 3, "3 rows returned"));
    return {};
}

DbErrorOr<void> select_order_by(Database& db) {
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number", "string" } },
                              "test",
                              {},
                              AST::OrderBy { .column_name = "number", .order = AST::OrderBy::Order::Ascending })
                              .execute(db))
                          .to_select_result());
    TRY(expect(TRY(result.rows()[1].value(1).to_int()) < TRY(result.rows()[5].value(1).to_int()), "values are sorted"));
    return {};
}

DbErrorOr<void> select_order_by_desc(Database& db) {
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number", "string" } },
                              "test",
                              {},
                              AST::OrderBy { .column_name = "number", .order = AST::OrderBy::Order::Descending })
                              .execute(db))
                          .to_select_result());
    TRY(expect(TRY(result.rows()[1].value(1).to_int()) > TRY(result.rows()[5].value(1).to_int()), "values are sorted"));
    return {};
}

DbErrorOr<void> select_top_number(Database& db) {
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number", "string" } },
                              "test",
                              {},
                              AST::OrderBy { .column_name = "number", .order = AST::OrderBy::Order::Descending },
                              AST::Top { .unit = AST::Top::Unit::Val, .value = 2 })
                              .execute(db))
                          .to_select_result());
    TRY(expect(result.rows().size() == 2, "select result is truncated to specified value"));
    return {};
}

DbErrorOr<void> select_top_perc(Database& db) {
    auto result = TRY(TRY(AST::Select(
                              { { "id", "number", "string" } },
                              "test",
                              {},
                              AST::OrderBy { .column_name = "number", .order = AST::OrderBy::Order::Descending },
                              AST::Top { .unit = AST::Top::Unit::Perc, .value = 75 })
                              .execute(db))
                          .to_select_result());
    TRY(expect_equal<size_t>(result.rows().size(), 4, "select result is truncated to specified value"));
    return {};
}

DbErrorOr<void> csv_export_import(Database& db) {
    auto table = db.table("test");
    table.release_value()->export_to_csv("test.csv");

    db.create_table("test2");
    table = db.table("test2");

    auto result = table.release_value()->import_from_csv("test.csv");

    return {};
}

using TestFunc = DbErrorOr<void>(Database&);

int main() {
    Db::Core::Database db;
    auto error = setup_db(db);
    if (error.is_error()) {
        std::cout << "Failed to setup db: " << error.release_error().message() << std::endl;
        return 1;
    }

    std::map<std::string, TestFunc*> funcs = {
        { "select_simple", select_simple },
        { "select_columns", select_columns },
        { "select_where", select_where },
        { "select_where_operator", select_where_operator },
        { "select_order_by", select_order_by },
        { "select_order_by_desc", select_order_by_desc },
        { "select_top_number", select_top_number },
        { "select_top_perc", select_top_perc },
        { "csv_export_import", csv_export_import }
    };

    for (auto const& func : funcs) {
        auto f = func.second(db);
        if (f.is_error()) {
            std::cout << "FAIL " << func.first << " " << f.release_error().message() << std::endl;
        }
        else {
            std::cout << "PASS " << func.first << std::endl;
        }
    }

    return 0;
}
