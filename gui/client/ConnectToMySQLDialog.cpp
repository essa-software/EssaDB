#include "ConnectToMySQLDialog.hpp"

namespace EssaDB {

void ConnectToMySQLDialog::on_init() {
    (void)load_from_eml_resource(resource_manager().require<EML::EMLResource>("ConnectToMySQL.eml"));

    m_address = find_widget_of_type_by_id_recursively<GUI::TextEditor>("address");
    m_port = find_widget_of_type_by_id_recursively<GUI::TextEditor>("port");
    m_port->set_content("3306");
    m_username = find_widget_of_type_by_id_recursively<GUI::TextEditor>("username");
    m_password = find_widget_of_type_by_id_recursively<GUI::TextEditor>("password");
    m_database = find_widget_of_type_by_id_recursively<GUI::TextEditor>("database");
}

Db::Core::DbErrorOr<uint16_t> port_from_string(std::string const& input) {
    try {
        auto number = std::stoi(input);
        if (number < 0 || number > 65536) {
            return Db::Core::DbError { "Port number out of range", 0 };
        }
        return static_cast<uint16_t>(number);
    } catch (...) {
        return Db::Core::DbError { "Port is not a number", 0 };
    }
}

Db::Core::DbErrorOr<MySQLConnectionData> ConnectToMySQLDialog::connection_data() const {
    return MySQLConnectionData {
        .address = m_address->content().encode(),
        .port = TRY(port_from_string(m_port->content().encode())),
        .username = m_username->content().encode(),
        .password = m_password->content().encode(),
        .database = m_database->content().encode(),
    };
}

}
