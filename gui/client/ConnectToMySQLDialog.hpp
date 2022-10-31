#pragma once

#include <EssaGUI/gui/FileExplorer.hpp>
#include <EssaGUI/gui/HostWindow.hpp>
#include <EssaGUI/gui/TextButton.hpp>
#include <EssaGUI/gui/TextEditor.hpp>
#include <EssaGUI/gui/ToolWindow.hpp>
#include <db/core/DbError.hpp>

namespace EssaDB {

struct MySQLConnectionData {
    std::string address;
    uint16_t port;
    std::string username;
    std::string password;
    std::string database;
};

class ConnectToMySQLDialog : public GUI::Container {
public:
    virtual void on_init() override;
    Db::Core::DbErrorOr<MySQLConnectionData> connection_data() const;

private:
    GUI::TextEditor* m_address = nullptr;
    GUI::TextEditor* m_port = nullptr;
    GUI::TextEditor* m_username = nullptr;
    GUI::TextEditor* m_password = nullptr;
    GUI::TextEditor* m_database = nullptr;
};

}
