#include "DatabaseModel.hpp"
#include "ImportCSVDialog.hpp"
#include "gui/ConnectToMySQLDialog.hpp"
#include <EssaGUI/eml/EMLResource.hpp>
#include <EssaGUI/gui/Application.hpp>
#include <EssaGUI/gui/Console.hpp>
#include <EssaGUI/gui/MessageBox.hpp>
#include <EssaGUI/gui/TreeView.hpp>
#include <algorithm>
#include <db/sql/SQL.hpp>
#include <iomanip>
#include <sstream>

#include <mysql.h>
#include <vector>

int mode = 0;

int main() {
    GUI::Application app;
    MYSQL* connection;

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
    auto console = container.find_widget_of_type_by_id_recursively<GUI::Console>("sql_console");

    Db::Core::Database db;
    auto& db_model = tree_view->create_and_set_model<EssaDB::DatabaseModel>(db);

    auto run_sql = [&](Util::UString const& query) {
        if (mode == 0) {
            console->append_content({ .color = Util::Color { 100, 100, 255 }, .text = "> " + query });
            auto result = Db::Sql::run_query(db, query.encode());
            if (result.is_error()) {
                console->append_content({ .color = Util::Colors::Red, .text = Util::UString { result.release_error().message() } });
            }
            else {
                std::ostringstream oss;
                result.release_value().repl_dump(oss);
                console->append_content({ .color = Util::Colors::White, .text = Util::UString { oss.str() } });
                db_model.update();
            }
        }
        else if (mode == 1) {
            MYSQL_RES *result;
            MYSQL_FIELD *field;
            MYSQL_ROW row;

            if (mysql_query(connection, query.encode().data())) {
                console->append_content({
                    .color = Util::Colors::Red,
                    .text = Util::UString {
                        fmt::format("Error querying server: {}", mysql_error(connection)) },
                });
            }
            
            result = mysql_use_result(connection);
            if (result == NULL) {
                console->append_content({
                    .color = Util::Colors::Lime,
                    .text = Util::UString {
                        fmt::format("Query executed successfully!") },
                });
            }else{
                std::vector<std::vector<std::string>> table_vector(1);

                int num_fields = mysql_num_fields(result);

                std::vector<unsigned> max_width(num_fields, 0);

                int i = 0;
                while((field = mysql_fetch_field(result))){
                    table_vector[0].push_back(field->name);
                    max_width[i] = std::max(max_width[i], field->name_length);
                    i++;
                }

                while((row = mysql_fetch_row(result))){
                    table_vector.push_back(std::vector<std::string>(num_fields));
                    for(int i = 0; i < num_fields; i++){
                        std::string str = row[i] ? row[i] : "NULL";
                        table_vector.back()[i] = str;
                        max_width[i] = std::max(max_width[i], (unsigned)str.size());
                    }
                }

                std::ostringstream oss;

                for(unsigned i = 0; i < table_vector.size(); i++){
                    for(int j = 0; j < num_fields; j++){
                        oss << "| " << table_vector[i][j] << std::setw(max_width[j] - table_vector[i][j].size());
                    }
                    oss << "|\n";
                }
                console->append_content({ .color = Util::Colors::White, .text = Util::UString { oss.str() } });
                mysql_free_result(result);
            }
        }
    };
// northwind_mysql
    run_button->on_click = [&]() { run_sql(text_editor->content()); };
    import_button->on_click = [&]() {
        auto& import_csv_dialog = window.open_overlay<EssaDB::ImportCSVDialog>();
        import_csv_dialog.on_ok = [&window, &db, &console, &import_csv_dialog, &db_model]() {
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
            db_model.update();
            return true;
        };
        import_csv_dialog.run();
    };

    connect_button->on_click = [&]() {
        if (mode == 1) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString {
                    fmt::format("You need to close the active conection in order to perform this operation!") },
            });
            return;
        }
        auto& connect_mysql_dialog = window.open_overlay<EssaDB::ConnectToMySQLDialog>();

        connect_mysql_dialog.on_ok = [&connection, &console, &connect_mysql_dialog]() {
            connection = mysql_init(NULL);
            if (connection == NULL) {
                console->append_content({
                    .color = Util::Colors::Red,
                    .text = Util::UString {
                        fmt::format("Failed to initialize: {}", mysql_error(connection)) },
                });
                return false;
            }

            int port = 0;

            try{
                port = std::stoi(connect_mysql_dialog.port());
            }catch(...){
                console->append_content({
                    .color = Util::Colors::Red,
                    .text = Util::UString {
                        fmt::format("Value in 'port' field is invalid!") },
                });
            }

            if (!mysql_real_connect(connection,
                    connect_mysql_dialog.address().data(),
                    connect_mysql_dialog.username().data(),
                    connect_mysql_dialog.password().data(),
                    connect_mysql_dialog.database().data(),
                    port, NULL, 0)) {

                console->append_content({
                    .color = Util::Colors::Red,
                    .text = Util::UString {
                        fmt::format("Failed to connect to server: {}", mysql_error(connection)) },
                });
                return false;
            }

            console->append_content({
                .color = Util::Colors::Lime,
                .text = Util::UString {
                    fmt::format("Successfully connected to the database '{}'!", connect_mysql_dialog.database()) },
            });

            mode = 1;

            return true;
        };

        connect_mysql_dialog.run();
    };

    close_connection->on_click = [&]() {
        if (mode == 0) {
            console->append_content({
                .color = Util::Colors::Red,
                .text = Util::UString {
                    fmt::format("Cannot perform this operation on a closed connection!") },
            });
        }
        else if (mode == 1) {
            mode = 0;
            mysql_close(connection);
            console->append_content({
                .color = Util::Colors::Lime,
                .text = Util::UString {
                    fmt::format("MySQL connection closed successfully!") },
            });
        }
    };

    text_editor->on_enter = [&](Util::UString const& query) { run_sql(query); };

    app.run();

    if (mode == 1) {
        mysql_close(connection);
    }

    return 0;
}
