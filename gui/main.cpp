#include <EssaGUI/eml/EMLResource.hpp>
#include <EssaGUI/gui/Application.hpp>
#include <EssaGUI/gui/Console.hpp>
#include <EssaGUI/gui/Container.hpp>
#include <EssaGUI/gui/FileExplorer.hpp>
#include <EssaGUI/gui/MessageBox.hpp>
#include <EssaGUI/gui/TextButton.hpp>
#include <EssaGUI/gui/TextEditor.hpp>
#include <EssaGUI/gui/Textbox.hpp>
#include <db/sql/SQL.hpp>
#include <sstream>

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
            };

            file_explorer_wnd.run();
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

int main() {
    GUI::Application app;
    auto& window = app.create_host_window({ 1200, 700 }, "EssaDB");

    auto& container = window.set_main_widget<GUI::Container>();
    container.set_layout<GUI::VerticalBoxLayout>();

    auto controls = container.add_widget<GUI::Container>();
    controls->set_size({ Length::Auto, { static_cast<float>(app.theme().line_height), Length::Unit::Px } });
    controls->set_layout<GUI::HorizontalBoxLayout>();

    auto run_button = controls->add_widget<GUI::TextButton>();
    run_button->set_content("Run SQL");
    run_button->set_size({ 120.0_px, Length::Auto });

    auto import_button = controls->add_widget<GUI::TextButton>();
    import_button->set_content("Import CSV...");
    import_button->set_size({ 120.0_px, Length::Auto });

    auto text_editor = container.add_widget<GUI::TextEditor>();
    [[maybe_unused]] auto console = container.add_widget<GUI::Console>();

    Db::Core::Database db;

    auto run_sql = [&](Util::UString const& query) {
        console->append_content({ .color = Util::Color { 100, 100, 255 }, .text = "> " + query });
        auto result = Db::Sql::run_query(db, query.encode());
        if (result.is_error()) {
            console->append_content({ .color = Util::Colors::Red, .text = Util::UString { result.release_error().message() } });
        }
        else {
            std::ostringstream oss;
            result.release_value().repl_dump(oss);
            console->append_content({ .color = Util::Colors::White, .text = Util::UString { oss.str() } });
        }
    };

    run_button->on_click = [&]() { run_sql(text_editor->content()); };
    import_button->on_click = [&]() {
        auto& import_csv_dialog = window.open_overlay<ImportCSVDialog>();
        import_csv_dialog.on_ok = [&window, &db, &console, &import_csv_dialog]() {
            auto maybe_error = db.import_to_table(import_csv_dialog.csv_file(),
                import_csv_dialog.table_name(), Db::Core::AST::Import::Mode::Csv);
            if (maybe_error.is_error()) {
                auto message = maybe_error.release_error().message();
                console->append_content({ .color = Util::Colors::Red, .text = Util::UString { message } });
                GUI::message_box(window, Util::UString { message }, "Error", GUI::MessageBox::Buttons::Ok);
                return false;
            }
            console->append_content({
                .color = Util::Colors::Lime,
                .text = Util::UString {
                    fmt::format("Successfully imported CSV {} to {}", import_csv_dialog.csv_file(), import_csv_dialog.table_name()) },
            });
            return true;
        };
        import_csv_dialog.run();
    };
    text_editor->on_enter = [&](Util::UString const& query) { run_sql(query); };

    app.run();
    return 0;
}
