#include "DatabaseModel.hpp"
#include "gui/Structure.hpp"

#include <Essa/GUI/Application.hpp>
#include <EssaUtil/Config.hpp>

namespace EssaDB {

namespace DBModelDataType {
static constexpr int Table = 0;
static constexpr int ColumnList = 1;
static constexpr int Column = 2;
}

size_t DatabaseModel::column_count() const {
    return 1;
}

GUI::ModelColumn DatabaseModel::column(size_t column) const {
    switch (column) {
    case 0:
        return { .width = 250, .name = "" };
    }
    ESSA_UNREACHABLE;
}

static std::string column_to_string(Structure::Column const& column) {
    // auto default_value = column.default_value();
    return fmt::format("{}", column.name);
    // TODO
    // (column.not_null() ? " NOT NULL" : ""),
    // (column.auto_increment() ? " AUTO_INCREMENT" : ""),
    // (column.unique() ? " UNIQUE" : ""),
    //(!default_value.is_null() ? fmt::format(" DEFAULT {}", default_value.to_string().release_value()) : ""));
}

static std::string value_type_icon(Db::Core::Value::Type type) {
    switch (type) {
    case Db::Core::Value::Type::Int:
        return "int";
    case Db::Core::Value::Type::Float:
        return "float";
    case Db::Core::Value::Type::Varchar:
        return "varchar";
    case Db::Core::Value::Type::Bool:
        return "bool";
    case Db::Core::Value::Type::Time:
        return "time";
    default:
        ESSA_UNREACHABLE;
    }
}

GUI::Variant DatabaseModel::data(Node const& node, size_t) const {
    switch (node.type) {
    case DBModelDataType::Table:
        return Util::UString { static_cast<Structure::Table const*>(node.data)->name };
    case DBModelDataType::ColumnList:
        return "Columns";
    case DBModelDataType::Column: {
        auto column = static_cast<Structure::Column const*>(node.data);
        return Util::UString { column_to_string(*column) };
    }
    }
    ESSA_UNREACHABLE;
}

llgl::Texture const* DatabaseModel::icon(Node const& node) const {
    switch (node.type) {
    case DBModelDataType::Table:
        return &GUI::Application::the().resource_manager().require_texture("gui/blockDevice.png");
    case DBModelDataType::Column: {
        auto column = static_cast<Structure::Column const*>(node.data);
        return &GUI::Application::the().resource_manager().require_texture("value_types/" + value_type_icon(column->type) + ".png");
    }
    default:
        return nullptr;
    }
}

size_t DatabaseModel::children_count(Node const* parent) const {
    if (!parent) {
        return m_structure.tables.size();
    }
    switch (parent->type) {
    case DBModelDataType::Table:
        return 1;
    case DBModelDataType::ColumnList:
        return static_cast<Structure::Table const*>(parent->data)->columns.size();
    case DBModelDataType::Column:
        return 0;
    }
    ESSA_UNREACHABLE;
}

GUI::Model::Node DatabaseModel::child(Node const* parent, size_t index) const {
    if (!parent) {
        return { .type = DBModelDataType::Table, .data = &m_structure.tables[index] };
    }
    switch (parent->type) {
    case DBModelDataType::Table:
        return { .type = DBModelDataType::ColumnList, .data = parent->data };
    case DBModelDataType::ColumnList: {
        auto table = static_cast<Structure::Table const*>(parent->data);
        return { .type = DBModelDataType::Column, .data = &table->columns[index] };
    }
    }
    ESSA_UNREACHABLE;
}

Db::Core::DbErrorOr<void> DatabaseModel::update(DatabaseClient const* client) {
    m_structure = client ? TRY(client->structure()) : Structure::Database {};
    return {};
}

}
