#pragma once

#include <db/core/Database.hpp>
#include <db/core/Value.hpp>

namespace Db::Sql {

Core::DbErrorOr<Core::ValueOrResultSet> run_query(Core::Database&, std::string const&);
void display_error(Core::DbError const& error, ssize_t error_start, std::string const& query);

}
