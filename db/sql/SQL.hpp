#pragma once

#include <db/core/Database.hpp>
#include <db/core/Value.hpp>

namespace Db::Sql {

Core::DbErrorOr<Core::Value> run_query(Core::Database&, std::string const&);
void display_error(Db::Core::DbError const& error, ssize_t error_start, std::string const& query);

}
