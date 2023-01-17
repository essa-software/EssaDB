#include <Essa/GUI/Application.hpp>
#include <Essa/GUI/EML/EMLResource.hpp>
#include <Essa/GUI/Overlays/MessageBox.hpp>
#include <Essa/GUI/Widgets/Console.hpp>
#include <Essa/GUI/Widgets/TreeView.hpp>
#include <EssaUtil/UString.hpp>
#include <algorithm>
#include <db/core/Table.hpp>
#include <db/core/Value.hpp>
#include <db/sql/Lexer.hpp>
#include <db/sql/SQL.hpp>
#include <gui/ConnectDialog.hpp>
#include <gui/DatabaseModel.hpp>
#include <gui/ImportCSVDialog.hpp>
#include <gui/SQLSyntaxHighlighter.hpp>
#include <gui/client/DatabaseClient.hpp>
#include <gui/client/EssaDBDatabaseClient.hpp>
#include <gui/client/MySQLDatabaseClient.hpp>
#include <iomanip>
#include <mysql.h>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

int main() {
    GUI::Application app;

    app.set_tps_limit(25);

    EssaDB::DatabaseClient::register_type("essadb", std::make_unique<EssaDB::EssaDBDatabaseClientType>());
    EssaDB::DatabaseClient::register_type("mysql", std::make_unique<EssaDB::MySQLDatabaseClientType>());

    std::unique_ptr<EssaDB::DatabaseClient> client;

    auto& window = app.create_host_window({ 1200, 700 }, "EssaDB");

    auto& container = window.set_main_widget<GUI::Container>();
    if (!container.load_from_eml_resource(app.resource_manager().require<EML::EMLResource>("MainWidget.eml")))
        return 1;

    auto run_button = container.find_widget_of_type_by_id_recursively<GUI::TextButton>("run_sql");
    auto import_button = container.find_widget_of_type_by_id_recursively<GUI::TextButton>("import_csv");
    auto connect_button = container.find_widget_of_type_by_id_recursively<GUI::TextButton>("connect_mysql");
    auto close_connection = container.find_widget_of_type_by_id_recursively<GUI::TextButton>("close_connection");
    auto outline_container = container.find_widget_of_type_by_id_recursively<GUI::Container>("outline_container");
    outline_container->set_background_color(app.theme().sidebar);

    auto tree_view = outline_container->add_widget<GUI::TreeView>();
    tree_view->set_display_header(false);

    auto text_editor = container.find_widget_of_type_by_id_recursively<GUI::TextEditor>("sql_query_editor");
    text_editor->set_syntax_highlighter(std::make_unique<EssaDB::SQLSyntaxHighlighter>());
    auto console = container.find_widget_of_type_by_id_recursively<GUI::Console>("sql_console");

    auto& db_model = tree_view->create_and_set_model<EssaDB::DatabaseModel>();

    auto update_db_model = [&]() {
        auto maybe_error = db_model.update(client.get());
        if (maybe_error.is_error()) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString { fmt::format("Failed to update database structure: {}", maybe_error.release_error().message()) },
            });
        }
    };

    auto run_sql = [&](Util::UString const& query) {
        if (!client) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString { "Connection to server is not opened" },
            });
            return;
        }

        console->append_content({ .color = Util::Color { 100, 100, 255 }, .text = "> " + query });
        auto result = client->run_query(query.encode());
        if (result.is_error()) {
            auto error = result.release_error();
            console->append_content({ .color = Util::Colors::Red, .text = Util::UString { error.message() } });

            // FIXME: Find a way to not lex twice. Possible solution to this
            //        is finally storing full code range in tokens.
            std::istringstream iss { query.encode() };
            Db::Sql::Lexer lexer { iss };
            auto tokens = lexer.lex();

            auto token = tokens[error.token()];
            auto token_start = text_editor->index_to_position(token.start);
            auto token_end = text_editor->index_to_position(token.end);
            if (token_end == GUI::TextPosition {}) {
                // FIXME: This is workaround for lexer bug that makes it set [0:0]
                //        as end of last token (for some reason).
                token_end = text_editor->index_to_position(text_editor->content().size());
            }
            if (token_start > token_end) {
                std::swap(token_start, token_end);
            }
            if (token_start == token_end) {
                token_end.column++;
            }
            fmt::print("{}:{} {}:{}\n", token_start.line, token_start.column, token_end.line, token_end.column);
            text_editor->set_error_spans({
                { GUI::TextEditor::ErrorSpan::Type::Error, { token_start, token_end } },
            });
        }
        else {
            std::ostringstream oss;
            // TODO: Support fancy dump
            result.release_value().repl_dump(oss, Db::Core::ResultSet::FancyDump::No);
            console->append_content({ .color = Util::Colors::White, .text = Util::UString { oss.str() } });
        }
        update_db_model();
        return;
    };

    run_button->on_click = [&]() { run_sql(text_editor->content()); };
    import_button->on_click = [&]() {
        if (!client) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString { "Connection to server is not opened" },
            });
            return;
        }

        auto& import_csv_dialog = window.open_overlay<EssaDB::ImportCSVDialog>();
        import_csv_dialog.on_ok = [&window, &console, &import_csv_dialog, &client, &update_db_model]() {
            auto maybe_error = client->import(import_csv_dialog.csv_file(), import_csv_dialog.table_name(), Db::Core::ImportMode::Csv);
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

            update_db_model();
            return true;
        };
        import_csv_dialog.show_modal();
        update_db_model();
    };

    connect_button->on_click = [&]() {
        if (client) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString { "You need to close the active connection in order to perform this operation!" },
            });
            return;
        }
        client = EssaDB::connect_to_user_selected_database(window);
        if (!client) {
            return;
        }
        console->append_content({
            .color = Util::Colors::White,
            .text = Util::UString { "Connected to " + client->status_string() },
        });
        window.window().set_title("EssaDB - " + client->status_string());
        update_db_model();
    };

    close_connection->on_click = [&]() {
        if (!client) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString {
                    fmt::format("Cannot perform this operation on a closed connection!") },
            });
            return;
        }
        client = nullptr;
        window.window().set_title("EssaDB");
        console->append_content({
            .color = Util::Colors::White,
            .text = Util::UString {
                fmt::format("Connection closed.") },
        });
        update_db_model();
    };

    text_editor->on_enter = [&](Util::UString const& query) { run_sql(query); };
    app.run();
    return 0;
}
