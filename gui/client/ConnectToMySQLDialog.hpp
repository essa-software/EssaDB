#pragma once

#include <Essa/GUI/Overlays/FileExplorer.hpp>
#include <Essa/GUI/HostWindow.hpp>
#include <Essa/GUI/Widgets/TextButton.hpp>
#include <Essa/GUI/Widgets/TextEditor.hpp>
#include <Essa/GUI/Overlays/ToolWindow.hpp>
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
