#pragma once

#include <Essa/GUI/HostWindow.hpp>
#include <Essa/GUI/Overlays/FileExplorer.hpp>
#include <Essa/GUI/Overlays/ToolWindow.hpp>
#include <Essa/GUI/Widgets/RadioGroup.hpp>
#include <Essa/GUI/Widgets/TextButton.hpp>
#include <Essa/GUI/Widgets/TextEditor.hpp>
#include <db/core/DbError.hpp>

namespace EssaDB {

struct EssaDBConnectionData {
    std::optional<std::string> path;
};

class ConnectToEssaDBDialog : public GUI::Container {
public:
    virtual void on_init() override;
    EssaDBConnectionData connection_data() const;

private:
    GUI::TextEditor* m_database_directory = nullptr;
    GUI::RadioGroup* m_memory_or_file_backed_radio_group = nullptr;
};

}
