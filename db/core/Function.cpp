#include "Function.hpp"
#include "db/core/Value.hpp"
#include <cctype>
#include <cstddef>

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

DbErrorOr<Value> Function::evaluate(EvaluationContext& context, Row const& row) const {
    if (compare_case_insensitive(m_name, "LEN")) {
        // https://www.w3schools.com/sqL/func_sqlserver_len.asp
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: string", start() + 1 };
        auto arg = TRY(m_args[0]->evaluate(context, row));
        // std::cout << arg.to_string().value() << "\n";
        switch (arg.type()) {
        case Value::Type::Null:
            // If string is NULL, it returns NULL
            return Value::null();
        default:
            // FIXME: What to do with ints?
            return Value::create_int(TRY(arg.to_string()).size());
        }
    }
    else if (compare_case_insensitive(m_name, "IN")) {
        if (m_args.size() == 0)
            return DbError { "No arguments were provided!", start() + 1 };
    }
    else if (compare_case_insensitive(m_name, "ASCII")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: char", start() + 1 };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        return Value::create_int(static_cast<int>(arg.to_string().value().front()));
    }
    else if (compare_case_insensitive(m_name, "CHAR")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: int", start() + 1 };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        std::string c = "";
        c += static_cast<char>(arg.to_int().value());

        return Value::create_varchar(c);
    }
    else if (compare_case_insensitive(m_name, "CHARINDEX")) {
        if (m_args.size() < 1)
            return DbError { "Expected arg 0: substr", start() + 1 };
        if (m_args.size() < 2)
            return DbError { "Expected arg 1: string", start() + 1 };

        auto substr = m_args[0]->to_string();
        auto str = TRY(m_args[1]->evaluate(context, row).value().to_string());
        size_t start = 0;

        if (m_args.size() == 3)
            start = std::stoi(m_args[2]->to_string());

        auto find_index = str.find(substr, start);

        if (find_index == std::string::npos)
            return Value::create_int(0);
        return Value::create_int(find_index);
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

    return DbError { "Undefined function: '" + m_name + "'", start() };
}
}
