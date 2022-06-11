#include "setup.hpp"
#include "db/core/DbError.hpp"

#include <db/util/Error.hpp>
#include <iostream>

using namespace Db::Core;

Db::Core::DbErrorOr<void> expect(bool b, std::string const& message) {
    if (!b) {
        std::cout << " [X] " << message << std::endl;
        return DbError { message, 0 };
    }
    std::cout << " [V] " << message << std::endl;
    return {};
}

extern std::map<std::string, TestFunc*> get_tests();

int main() {
    std::map<std::string, TestFunc*> funcs = get_tests();

    for (auto const& func : funcs) {
        auto f = func.second();
        if (f.is_error()) {
            std::cout << "\e[31;1mFAIL\e[m " << func.first << " " << f.release_error().message() << std::endl;
        }
        else {
            std::cout << "\e[32;1mPASS\e[m " << func.first << std::endl;
        }
    }

    return 0;
}
