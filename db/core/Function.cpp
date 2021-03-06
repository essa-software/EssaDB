#include "Function.hpp"

#include "Value.hpp"
#include "db/core/DbError.hpp"
#include "db/sql/Parser.hpp"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>

namespace Db::Core::AST {

class ArgumentList : public std::vector<Value> {
public:
    ArgumentList(std::vector<Value> values)
        : std::vector<Value>(std::move(values)) { }

    DbErrorOr<Value> get_required(size_t index, std::string const& name) const {
        if (size() <= index) {
            // TODO: Store location info
            return DbError { "Required argument " + std::to_string(index) + " `" + name + "` not given", 0 };
        }
        return (*this)[index];
    }

    Value get_optional(size_t index, Value alternative) const {
        if (size() <= index) {
            return alternative;
        }
        return (*this)[index];
    }
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
    register_sql_function("STR", [](ArgumentList args) -> DbErrorOr<Value> {
        // https://www.w3schools.com/sqL/func_sqlserver_len.asp
        auto string = TRY(TRY(args.get_required(0, "sth to convert")).to_string());
        // FIXME: What to do with ints?
        return Value::create_varchar(string);
    });
    register_sql_function("ASCII", [](ArgumentList args) -> DbErrorOr<Value> {
        // https://docs.microsoft.com/en-us/sql/t-sql/functions/ascii-transact-sql?view=sql-server-ver16
        auto arg = TRY(args.get_required(0, "char"));
        if (arg.is_null())
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
    register_sql_function("CONCAT", [](ArgumentList args) -> DbErrorOr<Value> {
        if (args.size() == 0)
            return DbError { "No arguments were provided!", 0 };
        std::string result = "";

        for (size_t i = 0; i < args.size(); i++) {
            auto str = TRY(TRY(args.get_required(i, "str")).to_string());

            result += str;
        }

        return Value::create_varchar(result);
    });
    register_sql_function("LOWER", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";

        for (const auto& c : str) {
            result += c + ((isalpha(c) && c <= 'Z') ? 32 : 0);
        }

        return Value::create_varchar(str);
    });
    register_sql_function("SUBSTRING", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "string")).to_string());
        auto start = TRY(TRY(args.get_required(1, "starting index")).to_int());
        auto len = TRY(TRY(args.get_required(2, "substring length")).to_int());

        return Value::create_varchar(str.substr(start, len));
    });
    register_sql_function("UPPER", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";

        for (const auto& c : str) {
            result += c - ((isalpha(c) && c >= 'a') ? 32 : 0);
        }

        return Value::create_varchar(str);
    });
    register_sql_function("LEFT", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto len = TRY(TRY(args.get_required(1, "len")).to_int());

        return Value::create_varchar(str.substr(0, len));
    });
    register_sql_function("RIGHT", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto len = TRY(TRY(args.get_required(1, "len")).to_int());

        return Value::create_varchar(str.substr(str.size() - len, len));
    });
    register_sql_function("LTRIM", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";

        bool con = 0;

        for (const auto& c : str) {
            if (!isspace(c))
                con = 1;

            if (con)
                result += c;
        }

        return Value::create_varchar(result);
    });
    register_sql_function("RTRIM", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "", temp = "";

        for (const auto& c : str) {
            temp += c;

            if (!isspace(c)) {
                result += temp;
                temp = "";
            }
        }

        return Value::create_varchar(result);
    });
    register_sql_function("TRIM", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "", temp = "";
        bool con = 0;

        for (const auto& c : str) {
            if (con)
                temp += c;

            if (!isspace(c)) {
                result += temp;
                temp = "";

                if (!con)
                    result += c;
                con = 1;
            }
        }

        return Value::create_varchar(result);
    });
    register_sql_function("REPLACE", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto substr = TRY(TRY(args.get_required(1, "replaced substr")).to_string());
        auto to_replace = TRY(TRY(args.get_required(2, "substr to replace")).to_string());

        std::string result = "";

        for (size_t i = 0; i < str.size(); i++) {
            if (str.substr(i, substr.size()) == substr) {
                result += to_replace;
                i += substr.size() - 1;
            }
            else
                result += str[i];
        }

        return Value::create_varchar(result);
    });
    register_sql_function("REPLICATE", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        size_t count = TRY(TRY(args.get_required(1, "number of times")).to_int());

        std::string result = "";

        for (size_t i = 0; i < count; i++) {
            result += str;
        }

        return Value::create_varchar(result);
    });
    register_sql_function("REVERSE", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";

        for (int i = str.size() - 1; i >= 0; i--) {
            result += str[i];
        }

        return Value::create_varchar(result);
    });
    register_sql_function("STUFF", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto index = TRY(TRY(args.get_required(1, "index")).to_int());
        auto len = TRY(TRY(args.get_required(2, "len")).to_int());
        auto substr = TRY(TRY(args.get_required(3, "to replace")).to_string());

        std::string result = str.substr(0, index) + substr + str.substr(index + len);

        return Value::create_varchar(result);
    });
    register_sql_function("TRANSLATE", [](ArgumentList args) -> DbErrorOr<Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto to_translate = TRY(TRY(args.get_required(1, "to translate")).to_string());
        auto translation = TRY(TRY(args.get_required(2, "translation")).to_string());

        std::string result = "", temp = "";

        for (const auto& c : str) {
            temp += c;
            if (isspace(c)) {
                if (temp.substr(0, temp.size() - 1) == to_translate)
                    temp = translation + " ";

                result += temp;
                temp = "";
            }
        }

        if (temp == to_translate)
            temp = to_translate;

        result += temp;

        return Value::create_varchar(result);
    });
    register_sql_function("ABS", [](ArgumentList args) -> DbErrorOr<Value> {
        auto value = TRY(args.get_required(0, "number"));

        if (value.type() == Value::Type::Int)
            return Value::create_int(std::abs(TRY(value.to_int())));
        if (value.type() == Value::Type::Float)
            return Value::create_float(std::fabs(TRY(value.to_float())));
        if (value.is_null())
            return Value::null();
        return DbError(TRY(value.to_string()) + " is not a valid type!", 0);
    });
    register_sql_function("ACOS", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::acos(a));
    });
    register_sql_function("ASIN", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::asin(a));
    });
    register_sql_function("ATAN", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::atan(a));
    });
    register_sql_function("ATN2", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());
        auto b = TRY(TRY(args.get_required(1, "number")).to_float());

        return Value::create_float(std::atan2(a, b));
    });
    register_sql_function("CEILING", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_int(std::ceil(a));
    });
    register_sql_function("COS", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::cos(a));
    });
    register_sql_function("COT", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(1.f / std::tan(a));
    });
    register_sql_function("DEGREES", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(a / M_PI * 180.f);
    });
    register_sql_function("EXP", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::exp(a));
    });
    register_sql_function("FLOOR", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_int(std::floor(a));
    });
    register_sql_function("LOG", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::log(a));
    });
    register_sql_function("LOG10", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::log10(a));
    });
    register_sql_function("PI", [](ArgumentList args) -> DbErrorOr<Value> {
        if (args.size() > 0)
            DbError("Arguments for 'PI' function are not valid!", 0);
        return Value::create_float(M_PI);
    });
    register_sql_function("POWER", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());
        auto b = TRY(TRY(args.get_required(1, "number")).to_float());

        return Value::create_float(std::pow(a, b));
    });
    register_sql_function("RAND", [](ArgumentList args) -> DbErrorOr<Value> {
        unsigned seed = time(NULL);
        if (args.size() == 1)
            seed = TRY(args[0].to_int());

        std::srand(seed);

        return Value::create_int(std::rand());
    });
    register_sql_function("ROUND", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_int(std::round(a));
    });
    register_sql_function("SIGN", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_int((a == 0) ? 0 : (a < 0 ? -1 : 1));
    });
    register_sql_function("SIN", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::sin(a));
    });
    register_sql_function("SQRT", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::sqrt(a));
    });
    register_sql_function("SQUARE", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(a * a);
    });
    register_sql_function("TAN", [](ArgumentList args) -> DbErrorOr<Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Value::create_float(std::tan(a));
    });
    register_sql_function("IFNULL", [](ArgumentList args) -> DbErrorOr<Value> {
        auto val = TRY(args.get_required(0, "value"));
        auto alternative = TRY(args.get_required(1, "alternative value"));

        if (val.is_null())
            return alternative;
        return val;
    });
}

DbErrorOr<Value> Function::evaluate(EvaluationContext& context, TupleWithSource const& row) const {
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

DbErrorOr<Value> AggregateFunction::evaluate(EvaluationContext& context, TupleWithSource const&) const {
    if (!context.row_group) {
        // TODO: Store more information about where user can't use aggregate function.
        return DbError { "Cannot use aggregate function here", start() };
    }
    auto result = TRY(aggregate(context, *context.row_group));
    return result;
}

DbErrorOr<Value> AggregateFunction::aggregate(EvaluationContext& context, std::span<Tuple const> rows) const {
    if (!context.table)
        return DbError { "Internal error: aggregate() called without table", start() };
    auto column = context.table->get_column(m_column);
    if (!column)
        return DbError { "Invalid column '" + m_column + "' used in aggregate function", start() };

    switch (m_function) {
    case Function::Count: {
        int count = 0;
        for (auto& row : rows) {
            if (row.value(column->index).type() != Value::Type::Null)
                count += 1;
        }
        return Value::create_int(count);
    }
    case Function::Sum: {
        float sum = 0;
        for (auto& row : rows) {
            sum += TRY(row.value(column->index).to_float());
        }

        return Value::create_float(sum);
    }
    case Function::Min: {
        float min = std::numeric_limits<float>::max();
        for (auto& row : rows) {
            min = std::min(min, TRY(row.value(column->index).to_float()));
        }

        return Value::create_float(min);
    }
    case Function::Max: {
        float max = std::numeric_limits<float>::min();
        for (auto& row : rows) {
            max = std::max(max, TRY(row.value(column->index).to_float()));
        }

        return Value::create_float(max);
    }
    case Function::Avg: {
        float sum = 0;
        size_t count = 0;
        for (auto& row : rows) {
            sum += TRY(row.value(column->index).to_int());
            count++;
        }

        return Value::create_float(sum / (count != 0 ? count : 1));
    }
    default:
        break;
    }
    __builtin_unreachable();
}

}
