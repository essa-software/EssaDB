#pragma once

#include "client/DatabaseClient.hpp"
#include <Essa/GUI/HostWindow.hpp>
#include <Essa/GUI/Overlays/ToolWindow.hpp>

namespace EssaDB {

class SelectConnectionTypeDialog : public GUI::ToolWindow {
public:
    explicit SelectConnectionTypeDialog(GUI::HostWindow&);

    std::optional<std::string_view> selected_database_type() const { return m_selected_database_type; }

private:
    std::optional<std::string_view> m_selected_database_type;
};

std::unique_ptr<DatabaseClient> connect_to_user_selected_database(GUI::HostWindow&);

}
