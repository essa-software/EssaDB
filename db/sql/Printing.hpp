#pragma once

#include <string>

namespace Db::Sql::Printing {

std::string escape_identifier(std::string const& id);
std::string escape_string_literal(std::string const& id);

}
