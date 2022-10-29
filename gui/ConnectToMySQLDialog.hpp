#pragma once

#include <EssaGUI/gui/FileExplorer.hpp>
#include <EssaGUI/gui/HostWindow.hpp>
#include <EssaGUI/gui/TextButton.hpp>
#include <EssaGUI/gui/TextEditor.hpp>
#include <EssaGUI/gui/ToolWindow.hpp>

namespace EssaDB {

class ConnectToMySQLDialog : public GUI::ToolWindow {
public:
    ConnectToMySQLDialog(GUI::HostWindow& window)
        : GUI::ToolWindow(window) {
        (void)load_from_eml_resource(resource_manager().require<EML::EMLResource>("ConnectToMySQL.eml"));

        auto container = static_cast<GUI::Container*>(main_widget());

        m_address = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("address");
        m_port = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("port");
        m_port->set_content("3306");
        m_username = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("username");
        m_password = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("password");
        m_database = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("database");
        auto submit_connect = container->find_widget_of_type_by_id_recursively<GUI::TextButton>("submit_connect");
        auto submit_cancel = container->find_widget_of_type_by_id_recursively<GUI::TextButton>("submit_cancel");

        submit_connect->on_click = [this]() {
            m_ok_clicked = true;
            if (on_ok && on_ok())
                this->close();
        };

        submit_cancel->on_click = [this]() {
            this->close();
        };
    }

    // Returns true if import succeeded, false otherwise.
    std::function<bool()> on_ok;

    std::string address() const { return m_address->content().encode(); }
    std::string port() const { return m_port->content().encode(); }
    std::string username() const { return m_username->content().encode(); }
    std::string password() const { return m_password->content().encode(); }
    std::string database() const { return m_database->content().encode(); }
    bool ok_clicked() const { return m_ok_clicked; }

private:
    GUI::TextEditor* m_address = nullptr;
    GUI::TextEditor* m_port = nullptr;
    GUI::TextEditor* m_username = nullptr;
    GUI::TextEditor* m_password = nullptr;
    GUI::TextEditor* m_database = nullptr;
    bool m_ok_clicked = false;
};

}
