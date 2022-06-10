#include "Function.hpp"

namespace Db::Core::AST {

bool compare_case_insensitive(const std::string& lhs, const std::string& rhs) {
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
            return DbError { "Expected arg 0: string" };
        auto arg = TRY(m_args[0]->evaluate(context, row));
        switch (arg.type()) {
        case Value::Type::Null:
            // If string is NULL, it returns NULL
            return Value::null();
        default:
            // FIXME: What to do with ints?
            return Value::create_int(TRY(arg.to_string()).size());
        }
    }else if (compare_case_insensitive(m_name, "ASCII")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: char" };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        return Value::create_int(static_cast<int>(arg.to_string().value().front()));
    }else if (compare_case_insensitive(m_name, "CHAR")) {
        if (m_args.size() != 1)
            return DbError { "Expected arg 0: int" };

        auto arg = TRY(m_args[0]->evaluate(context, row));

        std::string c = "";
        c += static_cast<char>(arg.to_int().value());

        return Value::create_varchar(c);
    }else if (compare_case_insensitive(m_name, "CHARINDEX")) {
        if (m_args.size() == 1)
            return DbError { "Expected arg 0: substr" };
        else if (m_args.size() == 2)
            return DbError { "Expected arg 1: string" };

        auto substr = TRY(m_args[0]->evaluate(context, row).value().to_string());
        auto str = TRY(m_args[1]->evaluate(context, row).value().to_string());
        size_t start = 0;

        if(m_args.size() == 3)
            start = TRY(m_args[1]->evaluate(context, row).value().to_int());
        
        auto find_index = str.find(substr, start);

        if(find_index == std::string::npos)
            return Value::create_int(0);
        return Value::create_int(find_index);
    }

    return DbError { "Undefined function: '" + m_name + "'" };
}
}
