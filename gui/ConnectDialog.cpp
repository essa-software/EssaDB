#include "ConnectDialog.hpp"

#include <Essa/GUI/Application.hpp>
#include <Essa/GUI/Overlays/MessageBox.hpp>
#include <Essa/GUI/Overlays/ToolWindow.hpp>
#include <Essa/GUI/Widgets/Container.hpp>
#include <Essa/GUI/Widgets/RadioButton.hpp>
#include <Essa/GUI/Widgets/RadioGroup.hpp>
#include <Essa/GUI/Widgets/TextButton.hpp>

namespace EssaDB {

SelectConnectionTypeDialog::SelectConnectionTypeDialog(GUI::WidgetTreeRoot& window)
    : GUI::WindowRoot(window) {
    window.setup("Select connection type", { 250, 120 }, {});
    window.center_on_screen();
    auto& container = set_main_widget<GUI::Container>();
    container.set_layout<GUI::VerticalBoxLayout>().set_padding(GUI::Boxi::all_equal(10));

    auto radio_group = container.add_widget<GUI::RadioGroup>();
    radio_group->set_layout<GUI::VerticalBoxLayout>().set_spacing(10);
    std::vector<std::string_view> database_type_ids;
    for (auto const& type : DatabaseClient::types()) {
        database_type_ids.push_back(type.first);
        auto* btn = radio_group->add_widget<GUI::RadioButton>();
        btn->set_caption(type.second->name());
    }

    auto submit_container = container.add_widget<GUI::Container>();
    auto& layout = submit_container->set_layout<GUI::HorizontalBoxLayout>();
    layout.set_spacing(10);
    layout.set_content_alignment(GUI::BoxLayout::ContentAlignment::BoxEnd);
    submit_container->set_size({ Util::Length::Auto, 30.0_px });

    auto cancel = submit_container->add_widget<GUI::TextButton>();
    cancel->set_size({ 100.0_px, Util::Length::Auto });
    cancel->set_content("Cancel");
    cancel->on_click = [this]() {
        close();
    };

    auto submit = submit_container->add_widget<GUI::TextButton>();
    submit->set_size({ 100.0_px, Util::Length::Auto });
    submit->set_content("OK");
    submit->on_click = [database_type_ids = std::move(database_type_ids), radio_group, this]() {
        m_selected_database_type = database_type_ids[radio_group->get_index()];
        close();
    };
}

std::unique_ptr<DatabaseClient> connect_to_user_selected_database(GUI::HostWindow& window) {
    auto select_connection_type = GUI::Application::the().open_host_window<SelectConnectionTypeDialog>();
    select_connection_type.window.show_modal();
    auto dbclient_type = select_connection_type.root.selected_database_type();
    if (!dbclient_type) {
        return nullptr;
    }

    auto settings_widget = DatabaseClient::create_settings_widget(*dbclient_type);
    std::unique_ptr<DatabaseClient> client;
    if (settings_widget) {
        auto& window = GUI::Application::the().create_host_window({ 500, 250 }, "Create " + DatabaseClient::types().at(*dbclient_type)->name() + " connection");
        window.center_on_screen();

        auto& container = window.set_root_widget<GUI::Container>();
        container.set_layout<GUI::VerticalBoxLayout>().set_padding(GUI::Boxi::all_equal(10));

        container.add_created_widget(settings_widget);

        auto submit_container = container.add_widget<GUI::Container>();
        auto& layout = submit_container->set_layout<GUI::HorizontalBoxLayout>();
        layout.set_spacing(10);
        layout.set_content_alignment(GUI::BoxLayout::ContentAlignment::BoxEnd);
        submit_container->set_size({ Util::Length::Auto, 30.0_px });

        auto cancel = submit_container->add_widget<GUI::TextButton>();
        cancel->set_size({ 100.0_px, Util::Length::Auto });
        cancel->set_content("Cancel");
        cancel->on_click = [&window]() {
            window.close();
        };

        auto submit = submit_container->add_widget<GUI::TextButton>();
        submit->set_size({ 100.0_px, Util::Length::Auto });
        submit->set_content("OK");
        submit->on_click = [&]() {
            auto maybe_client = DatabaseClient::create(*dbclient_type, settings_widget.get());
            if (maybe_client.is_error()) {
                GUI::message_box(window, Util::UString { maybe_client.release_error().message() }, "Error", GUI::MessageBox::Buttons::Ok);
                return;
            }
            client = maybe_client.release_value();
            window.close();
        };

        window.show_modal();
        return client;
    }

    // No settings widget = no "connect" window
    auto maybe_client = DatabaseClient::create(*dbclient_type, nullptr);
    if (maybe_client.is_error()) {
        GUI::message_box(window, Util::UString { maybe_client.release_error().message() }, "Error", GUI::MessageBox::Buttons::Ok);
    }
    return maybe_client.release_value();
}

}
