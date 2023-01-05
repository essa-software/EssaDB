#include "Printing.hpp"

namespace Db::Sql {

std::string Printing::escape_identifier(std::string const& id) {
    // FIXME: Actually escape special characters
    if (id.find(" ") != std::string::npos) {
        return "[" + id + "]";
    }
    return id;
}

}
