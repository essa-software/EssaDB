#pragma once

#include <Essa/GUI/HostWindow.hpp>
#include <Essa/GUI/Overlays/FileExplorer.hpp>
#include <Essa/GUI/Overlays/ToolWindow.hpp>
#include <Essa/GUI/Widgets/TextButton.hpp>
#include <Essa/GUI/Widgets/TextEditor.hpp>

namespace EssaDB {

class ImportCSVDialog : public GUI::ToolWindow {
public:
    ImportCSVDialog(GUI::HostWindow& window)
        : GUI::ToolWindow(window) {
        (void)load_from_eml_resource(resource_manager().require<EML::EMLResource>("ImportCSV.eml"));

        auto container = static_cast<GUI::Container*>(main_widget());

        m_table_name = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("table_name");
        m_csv_file = container->find_widget_of_type_by_id_recursively<GUI::TextEditor>("csv_file");
        auto load_file = container->find_widget_of_type_by_id_recursively<GUI::TextButton>("load_file");
        auto submit_ok = container->find_widget_of_type_by_id_recursively<GUI::TextButton>("submit_ok");
        auto submit_cancel = container->find_widget_of_type_by_id_recursively<GUI::TextButton>("submit_cancel");

        load_file->on_click = [&]() {
            auto& file_explorer_wnd = window.open_overlay<GUI::FileExplorer>();
            file_explorer_wnd.set_size({ 1000, 600 });
            file_explorer_wnd.center_on_screen();
            file_explorer_wnd.on_submit = [this](std::filesystem::path path) {
                m_csv_file->set_content(Util::UString { path.string() });
                if (m_table_name->is_empty()) {
                    m_table_name->set_content(Util::UString { path.stem().string() });
                }
            };

            file_explorer_wnd.show_modal();
        };

        submit_ok->on_click = [this]() {
            if (on_ok && on_ok())
                this->close();
        };

        submit_cancel->on_click = [this]() {
            this->close();
        };
    }

    // Returns true if import succeeded, false otherwise.
    std::function<bool()> on_ok;

    std::string table_name() const { return m_table_name->content().encode(); }
    std::string csv_file() const { return m_csv_file->content().encode(); }
    bool ok_clicked() const { return m_ok_clicked; }

private:
    GUI::TextEditor* m_table_name = nullptr;
    GUI::TextEditor* m_csv_file = nullptr;
    bool m_ok_clicked = false;
};

}
