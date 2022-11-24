#pragma once

#include <db/core/Database.hpp>
#include <db/core/Value.hpp>
#include <db/core/ValueOrResultSet.hpp>

namespace Db::Sql {

SQLErrorOr<Core::ValueOrResultSet> run_query(Core::Database&, std::string const&);
void display_error(SQLError const& error, ssize_t error_start, ssize_t error_end, std::string const& query);

}
