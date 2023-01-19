#include "ConnectToEssaDBDialog.hpp"

namespace EssaDB {

void ConnectToEssaDBDialog::on_init() {
    (void)load_from_eml_resource(resource_manager().require<EML::EMLResource>("ConnectToEssaDB.eml"));

    m_database_directory = find_widget_of_type_by_id_recursively<GUI::TextEditor>("database_directory");
    m_memory_or_file_backed_radio_group = find_widget_of_type_by_id_recursively<GUI::RadioGroup>("memory_or_file_backed");

    auto load_file = find_widget_of_type_by_id_recursively<GUI::TextButton>("load_file");
    load_file->on_click = [&]() {
        auto& file_explorer_wnd = host_window().open_overlay<GUI::FileExplorer>();
        file_explorer_wnd.set_size({ 1000, 600 });
        file_explorer_wnd.center_on_screen();
        file_explorer_wnd.on_submit = [this](std::filesystem::path path) {
            m_database_directory->set_content(Util::UString { path.string() });
        };

        file_explorer_wnd.show_modal();
    };
}

EssaDBConnectionData ConnectToEssaDBDialog::connection_data() const {
    return EssaDBConnectionData {
        .path = m_memory_or_file_backed_radio_group->get_index() == 1
            ? m_database_directory->content().encode()
            : std::optional<std::string> {}
    };
}

}
