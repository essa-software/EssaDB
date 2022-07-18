#include <EssaGUI/gui/Application.hpp>
#include <EssaGUI/gui/Console.hpp>
#include <EssaGUI/gui/Container.hpp>
#include <EssaGUI/gui/TextButton.hpp>
#include <EssaGUI/gui/TextEditor.hpp>
#include <db/sql/SQL.hpp>
#include <sstream>

int main() {
    GUI::Window window { { 500, 500 }, "EssaDB" };

    GUI::Application app { window };

    auto& container = app.set_main_widget<GUI::Container>();
    container.set_layout<GUI::VerticalBoxLayout>();

    auto controls = container.add_widget<GUI::Container>();
    controls->set_size({ Length::Auto, 30.0_px });
    controls->set_layout<GUI::HorizontalBoxLayout>();

    auto run_button = controls->add_widget<GUI::TextButton>();
    run_button->set_content("Run SQL");
    run_button->set_size({ 100.0_px, Length::Auto });

    auto text_editor = container.add_widget<GUI::TextEditor>();
    [[maybe_unused]] auto console = container.add_widget<GUI::Console>();

    Db::Core::Database db;

    run_button->on_click = [&]() {
        auto query = text_editor->get_content();
        console->append_content({ .color = Util::Color { 100, 100, 255 }, .text = "> " + query });
        auto result = Db::Sql::run_query(db, query.encode());
        if (result.is_error()) {
            console->append_content({ .color = Util::Colors::red, .text = Util::UString { result.release_error().message() } });
        }
        else {
            std::ostringstream oss;
            result.release_value().repl_dump(oss);
            console->append_content({ .color = Util::Colors::white, .text = Util::UString { oss.str() } });
        }
    };

    app.run();
    return 0;
}
