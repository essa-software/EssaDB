#pragma once

#include "ResultSet.hpp"
#include "Value.hpp"

#include <variant>

namespace Db::Core {

namespace Detail {

using VORSVariant = std::variant<Value, ResultSet>;

}

class ValueOrResultSet : public Detail::VORSVariant {
public:
    ValueOrResultSet(Value v)
        : Detail::VORSVariant(std::move(v)) { }

    ValueOrResultSet(ResultSet rs)
        : Detail::VORSVariant(std::move(rs)) { }

    bool is_value() const { return std::holds_alternative<Value>(*this); }
    Value as_value() const { return std::get<Value>(*this); }

    bool is_result_set() const { return std::holds_alternative<ResultSet>(*this); }
    ResultSet as_result_set() const { return std::get<ResultSet>(*this); }

    void repl_dump(std::ostream& out) const {
        if (is_value()) {
            as_value().repl_dump(out);
        }
        else {
            as_result_set().dump(out);
        }
    }
};

}
