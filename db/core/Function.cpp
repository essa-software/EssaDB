#include "Function.hpp"

#include "Value.hpp"

#include <cctype>
#include <cstddef>
#include <functional>

namespace Db::Core::AST {

bool compare_case_insensitive(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size())
        return false;

    for (auto l = lhs.begin(), r = rhs.begin(); l != lhs.end() && r != rhs.end(); l++, r++) {
        char c1 = (*l > 97) ? *l - 32 : *l;
        char c2 = (*r > 97) ? *r - 32 : *r;

        if (c1 != c2)
            return false;
    }

    return true;
}

class ArgumentList {
public:
    ArgumentList(std::vector<Value> values)
        : m_args(std::move(values)) { }

    Value operator[](size_t index) const { return m_args[index]; }

    DbErrorOr<Value> get_required(size_t index, std::string const& name) const {
        if (m_args.size() <= index) {
            // TODO: Store location info
            return DbError { "Required argument " + std::to_string(index) + " `" + name + "` not given", 0 };
        }
        return m_args[index];
    }

    Value get_optional(size_t index, Value alternative) const {
        if (m_args.size() <= index) {
            return alternative;
        }
        return m_args[index];
    }

private:
    std::vector<Value> m_args;
};

using SQLFunction = std::function<DbErrorOr<Value>(ArgumentList)>;

static std::map<std::string, SQLFunction> s_functions;

static void register_sql_function(std::string name, SQLFunction function) {
    s_functions.insert({ std::move(name), std::move(function) });
}

static void setup_sql_functions() {
    register_sql_function("LEN", [](ArgumentList args) -> DbErrorOr<Value> {
        // https://www.w3schools.com/sqL/func_sqlserver_len.asp
        auto string = TRY(args.get_required(0, "string"));
        switch (string.type()) {
        case Value::Type::Null:
            // If string is NULL, it returns NULL
            return Value::null();
        default:
            // FIXME: What to do with ints?
            return Value::create_int(TRY(string.to_string()).size());
        }
    });
    register_sql_function("ASCII", [](ArgumentList args) -> DbErrorOr<Value> {
        // https://docs.microsoft.com/en-us/sql/t-sql/functions/ascii-transact-sql?view=sql-server-ver16
        auto arg = TRY(args.get_required(0, "char"));
        if (arg.type() == Value::Type::Null)
            return Value::null();
        auto string_ = TRY(arg.to_string());
        if (string_.size() < 1) {
            // TODO: Store location info
            return DbError { "Input string must be at least 1 character long", 0 };
        }
        return Value::create_int(static_cast<int>(string_[0]));
    });
    register_sql_function("CHAR", [](ArgumentList args) -> DbErrorOr<Value> {
        std::string c;
        c += static_cast<char>(TRY(TRY(args.get_required(0, "int")).to_int()));
        return Value::create_varchar(c);
    });
    register_sql_function("CHARINDEX", [](ArgumentList args) -> DbErrorOr<Value> {
        // https://docs.microsoft.com/en-us/sql/t-sql/functions/charindex-transact-sql?view=sql-server-ver16
        auto substr = TRY(TRY(args.get_required(0, "substr")).to_string());
        auto str = TRY(TRY(args.get_required(1, "str")).to_string());
        auto start = TRY(args.get_optional(2, Value::create_int(0)).to_int());

        auto find_index = str.find(substr, start);

        if (find_index == std::string::npos)
            return Value::null();
        return Value::create_int(find_index);
    });
}

DbErrorOr<Value> Function::evaluate(EvaluationContext& context, Row const& row) const {
    // TODO: Port all these to new register_sql_function API
    if (compare_case_insensitive(m_name, "IN")) {
        if (m_args.size() == 0)
            return DbError { "No arguments were provided!", start() + 1 };
    }
    else if (compare_case_insensitive(m_name, "CONCAT")) {
        if (m_args.size() == 0)
            return DbError { "No arguments were provided!", start() + 1 };
        std::string str = "";

        for (const auto& arg : m_args) {
            auto column = arg->evaluate(context, row);

            if (column.is_error())
                str += arg->to_string();
            else
                str += column.value().to_string().value();
        }

        return Value::create_varchar(str);
    }
    else if (compare_case_insensitive(m_name, "DATALENGTH")) {
    }
    else if (compare_case_insensitive(m_name, "DIFFERENCE")) {
    }
    else if (compare_case_insensitive(m_name, "FORMAT")) {
    }
    else if (compare_case_insensitive(m_name, "LEFT")) {
    }
    else if (compare_case_insensitive(m_name, "LOWER")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: string", start() + 1 };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        std::string str = "";

        for (const auto& c : arg.to_string().release_value()) {
            str += c + ((isalpha(c) && c <= 97) ? 32 : 0);
        }

        return Value::create_varchar(str);
    }
    else if (compare_case_insensitive(m_name, "LTRIM")) {
    }
    else if (compare_case_insensitive(m_name, "NCHAR")) {
    }
    else if (compare_case_insensitive(m_name, "PATINDEX")) {
    }
    else if (compare_case_insensitive(m_name, "REPLACE")) {
    }
    else if (compare_case_insensitive(m_name, "REPLICATE")) {
    }
    else if (compare_case_insensitive(m_name, "REVERSE")) {
    }
    else if (compare_case_insensitive(m_name, "RIGHT")) {
    }
    else if (compare_case_insensitive(m_name, "RTRIM")) {
    }
    else if (compare_case_insensitive(m_name, "SOUNDEX")) {
    }
    else if (compare_case_insensitive(m_name, "SPACE")) {
    }
    else if (compare_case_insensitive(m_name, "STR")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: int", start() + 1 };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        return Value::create_varchar(arg.to_string().value());
    }
    else if (compare_case_insensitive(m_name, "STUFF")) {
    }
    else if (compare_case_insensitive(m_name, "SUBSTRING")) {
        if (m_args.size() == 0)
            return DbError { "Expected arg 0: string", start() + 1 };
        else if (m_args.size() == 1)
            return DbError { "Expected arg 1: starting index", start() + 1 };
        else if (m_args.size() == 2)
            return DbError { "Expected arg 2: substring length", start() + 1 };

        auto arg = TRY(m_args[0]->evaluate(context, row));
        size_t start = std::stoi(m_args[1]->to_string());
        size_t len = std::stoi(m_args[2]->to_string());

        return Value::create_varchar(arg.to_string().value().substr(start, len));
    }
    else if (compare_case_insensitive(m_name, "TRANSLATE")) {
    }
    else if (compare_case_insensitive(m_name, "TRIM")) {
    }
    else if (compare_case_insensitive(m_name, "UPPER")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: string", start() + 1 };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        std::string str = "";

        for (const auto& c : arg.to_string().release_value()) {
            str += c - ((isalpha(c) && c >= 'a') ? 32 : 0);
        }

        return Value::create_varchar(str);
    }

    static bool sql_functions_setup = false;
    if (!sql_functions_setup) {
        setup_sql_functions();
        sql_functions_setup = true;
    }

    std::string name_uppercase;
    for (auto ch : m_name)
        name_uppercase += toupper(ch);

    auto function = s_functions.find(name_uppercase);
    if (function != s_functions.end()) {
        std::vector<Value> args;
        for (auto const& arg : m_args)
            args.push_back(TRY(arg->evaluate(context, row)));
        return function->second(ArgumentList { std::move(args) });
    }

    return DbError { "Undefined function: '" + m_name + "'", start() };
}
}
