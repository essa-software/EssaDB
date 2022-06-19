#include <tests/setup.hpp>

#include <db/core/AST.hpp>
#include <db/core/Database.hpp>
#include <db/core/SelectResult.hpp>
#include <db/sql/SQL.hpp>

#include <iostream>
#include <string>
#include <utility>

using namespace Db::Core;

DbErrorOr<void> tuple_less_than() {
    // TODO: Port this to SQL
    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(2) },
        true, "(1) < (2)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(1) },
        false, "(1) <! (1)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(1) },
        false, "(1) <! (1)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1), Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(1), Value::create_int(2) },
        true, "(1, 1) < (1, 2)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1), Value::create_int(2) }
            < Db::Core::Tuple { Value::create_int(1), Value::create_int(1) },
        false, "(1, 2) <! (1, 1)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1), Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(2) },
        true, "(1, 1) < (2)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(2), Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(1) },
        false, "(2, 1) <! (1)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1), Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(1) },
        false, "(1, 1) <! (1)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(1) }
            < Db::Core::Tuple { Value::create_int(1), Value::create_int(2) },
        true, "(1) < (1, 2)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(2) }
            < Db::Core::Tuple { Value::create_int(1), Value::create_int(2) },
        false, "(2) <! (1, 2)"));

    TRY(expect_equal(Db::Core::Tuple { Value::create_int(2) }
            < Db::Core::Tuple { Value::create_int(2), Value::create_int(2) },
        true, "(2) < (2, 2)"));

    return {};
}

std::map<std::string, TestFunc> get_tests() {
    return {
        { "tuple_less_than", tuple_less_than }
    };
}
