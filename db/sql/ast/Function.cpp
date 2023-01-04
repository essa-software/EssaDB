#include "Function.hpp"

#include <EssaUtil/ScopeGuard.hpp>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <db/core/DbError.hpp>
#include <db/core/Value.hpp>
#include <db/sql/Parser.hpp>
#include <functional>
#include <limits>

namespace Db::Sql::AST {

class ArgumentList : public std::vector<Core::Value> {
public:
    ArgumentList(std::vector<Core::Value> values)
        : std::vector<Core::Value>(std::move(values)) { }

    Core::DbErrorOr<Core::Value> get_required(size_t index, std::string const& name) const {
        if (size() <= index) {
            return Core::DbError { "Required argument " + std::to_string(index) + " `" + name + "` not given" };
        }
        return (*this)[index];
    }

    Core::Value get_optional(size_t index, Core::Value alternative) const {
        if (size() <= index) {
            return alternative;
        }
        return (*this)[index];
    }
};

using SQLFunction = std::function<Core::DbErrorOr<Core::Value>(ArgumentList)>;

static std::map<std::string, SQLFunction> s_functions;

static void register_sql_function(std::string name, SQLFunction function) {
    s_functions.insert({ std::move(name), std::move(function) });
}

static void register_sql_function_alias(std::string name, std::string target) {
    s_functions.insert({ std::move(name), s_functions[target] });
}

static void setup_sql_functions() {
    register_sql_function("LEN", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        // https://www.w3schools.com/sqL/func_sqlserver_len.asp
        auto string = TRY(args.get_required(0, "string"));
        switch (string.type()) {
        case Core::Value::Type::Null:
            // If string is NULL, it returns NULL
            return Core::Value::null();
        default:
            // FIXME: What to do with ints?
            return Core::Value::create_int(TRY(string.to_string()).size());
        }
    });
    register_sql_function_alias("LENGTH", "LEN");
    register_sql_function("STR", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        // https://www.w3schools.com/sqL/func_sqlserver_len.asp
        auto string = TRY(TRY(args.get_required(0, "sth to convert")).to_string());
        // FIXME: What to do with ints?
        return Core::Value::create_varchar(string);
    });
    register_sql_function("ASCII", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        // https://docs.microsoft.com/en-us/sql/t-sql/functions/ascii-transact-sql?view=sql-server-ver16
        auto arg = TRY(args.get_required(0, "char"));
        if (arg.is_null())
            return Core::Value::null();
        auto string_ = TRY(arg.to_string());
        if (string_.size() < 1) {
            return Core::DbError { "Input string must be at least 1 character long" };
        }
        return Core::Value::create_int(static_cast<int>(string_[0]));
    });
    register_sql_function("CHAR", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        std::string c;
        c += static_cast<char>(TRY(TRY(args.get_required(0, "int")).to_int()));
        return Core::Value::create_varchar(c);
    });
    register_sql_function("CHARINDEX", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        // https://docs.microsoft.com/en-us/sql/t-sql/functions/charindex-transact-sql?view=sql-server-ver16
        auto substr = TRY(TRY(args.get_required(0, "substr")).to_string());
        auto str = TRY(TRY(args.get_required(1, "str")).to_string());
        auto start = TRY(args.get_optional(2, Core::Value::create_int(0)).to_int());

        auto find_index = str.find(substr, start);

        if (find_index == std::string::npos)
            return Core::Value::null();
        return Core::Value::create_int(find_index);
    });
    register_sql_function("CONCAT", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        if (args.size() == 0)
            return Core::DbError { "CONCAT requires at least one argument" };
        std::string result = "";

        for (size_t i = 0; i < args.size(); i++) {
            auto str = TRY(TRY(args.get_required(i, "str")).to_string());

            result += str;
        }

        return Core::Value::create_varchar(result);
    });
    register_sql_function("LOWER", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";
        for (const auto& c : str) {
            result += tolower(c);
        }
        return Core::Value::create_varchar(result);
    });
    register_sql_function("SUBSTRING", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "string")).to_string());
        auto start = TRY(TRY(args.get_required(1, "starting index")).to_int());
        auto len = TRY(TRY(args.get_required(2, "substring length")).to_int());

        return Core::Value::create_varchar(str.substr(start, len));
    });
    register_sql_function("UPPER", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";
        for (auto c : str) {
            result += toupper(c);
        }
        return Core::Value::create_varchar(result);
    });
    register_sql_function("LEFT", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto len = TRY(TRY(args.get_required(1, "len")).to_int());

        return Core::Value::create_varchar(str.substr(0, len));
    });
    register_sql_function("RIGHT", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto len = TRY(TRY(args.get_required(1, "len")).to_int());

        return Core::Value::create_varchar(str.substr(str.size() - len, len));
    });
    register_sql_function("LTRIM", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";

        bool con = 0;

        for (const auto& c : str) {
            if (!isspace(c))
                con = 1;

            if (con)
                result += c;
        }

        return Core::Value::create_varchar(result);
    });
    register_sql_function("RTRIM", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "", temp = "";

        for (const auto& c : str) {
            temp += c;

            if (!isspace(c)) {
                result += temp;
                temp = "";
            }
        }

        return Core::Value::create_varchar(result);
    });
    register_sql_function("TRIM", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
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

        return Core::Value::create_varchar(result);
    });
    register_sql_function("REPLACE", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
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

        return Core::Value::create_varchar(result);
    });
    register_sql_function("REPLICATE", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        size_t count = TRY(TRY(args.get_required(1, "number of times")).to_int());

        std::string result = "";

        for (size_t i = 0; i < count; i++) {
            result += str;
        }

        return Core::Value::create_varchar(result);
    });
    register_sql_function("REVERSE", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());

        std::string result = "";

        for (int i = str.size() - 1; i >= 0; i--) {
            result += str[i];
        }

        return Core::Value::create_varchar(result);
    });
    register_sql_function("STUFF", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto str = TRY(TRY(args.get_required(0, "str")).to_string());
        auto index = TRY(TRY(args.get_required(1, "index")).to_int());
        auto len = TRY(TRY(args.get_required(2, "len")).to_int());
        auto substr = TRY(TRY(args.get_required(3, "to replace")).to_string());

        std::string result = str.substr(0, index) + substr + str.substr(index + len);

        return Core::Value::create_varchar(result);
    });
    register_sql_function("TRANSLATE", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
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

        return Core::Value::create_varchar(result);
    });
    register_sql_function("ABS", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto value = TRY(args.get_required(0, "number"));

        if (value.type() == Core::Value::Type::Int)
            return Core::Value::create_int(std::abs(TRY(value.to_int())));
        if (value.type() == Core::Value::Type::Float)
            return Core::Value::create_float(std::fabs(TRY(value.to_float())));
        if (value.is_null())
            return Core::Value::null();
        return Core::DbError(TRY(value.to_string()) + " is not a valid type!");
    });
    register_sql_function("ACOS", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::acos(a));
    });
    register_sql_function("ASIN", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::asin(a));
    });
    register_sql_function("ATAN", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::atan(a));
    });
    register_sql_function("ATN2", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());
        auto b = TRY(TRY(args.get_required(1, "number")).to_float());

        return Core::Value::create_float(std::atan2(a, b));
    });
    register_sql_function("CEILING", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_int(std::ceil(a));
    });
    register_sql_function("COS", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::cos(a));
    });
    register_sql_function("COT", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(1.f / std::tan(a));
    });
    register_sql_function("DEGREES", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(a / M_PI * 180.f);
    });
    register_sql_function("EXP", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::exp(a));
    });
    register_sql_function("FLOOR", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_int(std::floor(a));
    });
    register_sql_function("LOG", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::log(a));
    });
    register_sql_function("LOG10", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::log10(a));
    });
    register_sql_function("PI", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        if (args.size() > 0)
            return Core::DbError("Arguments for 'PI' function are not valid!");
        return Core::Value::create_float(M_PI);
    });
    register_sql_function("POWER", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());
        auto b = TRY(TRY(args.get_required(1, "number")).to_float());

        return Core::Value::create_float(std::pow(a, b));
    });
    register_sql_function("RAND", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        unsigned seed = time(NULL);
        if (args.size() == 1)
            seed = TRY(args[0].to_int());

        std::srand(seed);

        return Core::Value::create_int(std::rand());
    });
    register_sql_function("ROUND", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_int(std::round(a));
    });
    register_sql_function("SIGN", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_int((a == 0) ? 0 : (a < 0 ? -1 : 1));
    });
    register_sql_function("SIN", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::sin(a));
    });
    register_sql_function("SQRT", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::sqrt(a));
    });
    register_sql_function("SQUARE", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(a * a);
    });
    register_sql_function("TAN", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto a = TRY(TRY(args.get_required(0, "number")).to_float());

        return Core::Value::create_float(std::tan(a));
    });
    register_sql_function("IFNULL", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto val = TRY(args.get_required(0, "value"));
        auto alternative = TRY(args.get_required(1, "alternative value"));

        if (val.is_null())
            return alternative;
        return val;
    });

    // Time functions
    register_sql_function("DATEDIFF", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        auto start = TRY(TRY(args.get_required(0, "start")).to_time()).to_utc_epoch();
        auto end = TRY(TRY(args.get_required(1, "end")).to_time()).to_utc_epoch();
        return Core::Value::create_int((end - start) / (24 * 60 * 60));
    });
    register_sql_function("DAY", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        return Core::Value::create_int(TRY(TRY(args.get_required(0, "date")).to_time()).day);
    });
    register_sql_function("MONTH", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        return Core::Value::create_int(TRY(TRY(args.get_required(0, "date")).to_time()).month);
    });
    register_sql_function("YEAR", [](ArgumentList args) -> Core::DbErrorOr<Core::Value> {
        return Core::Value::create_int(TRY(TRY(args.get_required(0, "date")).to_time()).year);
    });
}

SQLErrorOr<Core::Value> Function::evaluate(EvaluationContext& context) const {
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
        std::vector<Core::Value> args;
        for (auto const& arg : m_args)
            args.push_back(TRY(arg->evaluate(context)));
        return function->second(ArgumentList { std::move(args) }).map_error(DbToSQLError { start() });
    }

    return SQLError { "Undefined function: '" + m_name + "'", start() };
}

std::string Function::to_string() const {
    std::string str = m_name + "(";
    for (size_t s = 0; s < m_args.size(); s++) {
        str += m_args[s]->to_string();
        if (s != m_args.size() - 1) {
            str += ", ";
        }
    }
    str += ")";
    return str;
}

SQLErrorOr<Core::Value> AggregateFunction::evaluate(EvaluationContext& context) const {
    auto& frame = context.current_frame();
    if (frame.row_group) {
        return TRY(aggregate(context, *frame.row_group));
    }

    Core::Tuple tuple { {} };
    return TRY(aggregate(context, { &tuple, 1 }));
}

SQLErrorOr<Core::Value> AggregateFunction::aggregate(EvaluationContext& context, std::span<Core::Tuple const> rows) const {
    auto& frame = context.frames.emplace_back(context.current_frame().table, context.current_frame().columns);
    Util::ScopeGuard guard { [&] { context.frames.pop_back(); } };

    switch (m_function) {
    case Function::Count: {
        int count = 0;
        for (auto& row : rows) {
            frame.row = { .tuple = row, .source = {} };
            auto value = TRY(m_expression->evaluate(context));
            if (value.type() != Core::Value::Type::Null)
                count += 1;
        }
        return Core::Value::create_int(count);
    }
    case Function::Sum: {
        float sum = 0;
        for (auto& row : rows) {
            frame.row = { .tuple = row, .source = {} };
            auto value = TRY(m_expression->evaluate(context));
            sum += TRY(value.to_float().map_error(DbToSQLError { start() }));
        }

        return Core::Value::create_float(sum);
    }
    case Function::Min: {
        float min = std::numeric_limits<float>::max();
        for (auto& row : rows) {
            frame.row = { .tuple = row, .source = {} };
            auto value = TRY(m_expression->evaluate(context));
            min = std::min(min, TRY(value.to_float().map_error(DbToSQLError { start() })));
        }

        return Core::Value::create_float(min);
    }
    case Function::Max: {
        float max = std::numeric_limits<float>::min();
        for (auto& row : rows) {
            frame.row = { .tuple = row, .source = {} };
            auto value = TRY(m_expression->evaluate(context));
            max = std::max(max, TRY(value.to_float().map_error(DbToSQLError { start() })));
        }

        return Core::Value::create_float(max);
    }
    case Function::Avg: {
        float sum = 0;
        size_t count = 0;
        for (auto& row : rows) {
            frame.row = { .tuple = row, .source = {} };
            auto value = TRY(m_expression->evaluate(context));
            sum += TRY(value.to_int().map_error(DbToSQLError { start() }));
            count++;
        }

        return Core::Value::create_float(sum / (count != 0 ? count : 1));
    }
    default:
        break;
    }
    __builtin_unreachable();
}

std::string AggregateFunction::to_string() const {
    std::string str;
    switch (m_function) {
    case Function::Count:
        str += "COUNT";
        break;
    case Function::Sum:
        str += "SUM";
        break;
    case Function::Min:
        str += "MIN";
        break;
    case Function::Max:
        str += "MAX";
        break;
    case Function::Avg:
        str += "AVG";
        break;
    case Function::Invalid:
        str += "INVALID";
        break;
    }
    str += "(";
    str += m_expression->to_string();
    str += ")";
    return str;
}

}
