#include "setup.hpp"

#include <db/core/DbError.hpp>
#include <iostream>

using namespace Db::Core;

Db::Core::DbErrorOr<void> expect(bool b, std::string const& message) {
    if (!b) {
        std::cout << " [X] " << message << std::endl;
        return DbError { message };
    }
    std::cout << " [V] " << message << std::endl;
    return {};
}

extern std::map<std::string, TestFunc> get_tests();

bool run_test(std::pair<std::string, TestFunc> const& test) {
    std::cout << "\r\x1b[2K\e[1m .. \e[m " << test.first << std::flush;
    auto f = test.second();
    if (f.is_error()) {
        std::cout << "\r\x1b[2K\e[31;1mFAIL\e[m " << test.first << " " << f.release_error().message() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::map<std::string, TestFunc> funcs = get_tests();

    if (argc == 2) {
        if (std::string_view { argv[1] } == "list") {
            for (auto const& func : funcs) {
                std::cout << func.first << std::endl;
            }
            return 0;
        }
        auto test_to_run = funcs.find(argv[1]);
        if (test_to_run == funcs.end()) {
            std::cout << "\e[31;1mFAIL\e[m Test not found: " << argv[1] << std::endl;
            return 1;
        }
        return run_test(*test_to_run) ? 0 : 1;
    }

    bool success = true;

    for (auto const& func : funcs) {
        success &= run_test(func);
    }

    std::cout << "\r\x1b[2K";
    return success ? 0 : 1;
}
