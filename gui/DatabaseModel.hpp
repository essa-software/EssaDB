#pragma once

#include <EssaGUI/gui/Model.hpp>
#include <db/core/Database.hpp>

namespace EssaDB {

class DatabaseModel : public GUI::Model {
public:
    explicit DatabaseModel(Db::Core::Database const& db)
        : m_db(db) { }

    void update();

private:
    virtual size_t column_count() const override;
    virtual GUI::ModelColumn column(size_t column) const override;
    virtual GUI::Variant data(Node const&, size_t column) const override;
    virtual llgl::Texture const* icon(Node const&) const override;
    virtual size_t children_count(Node const*) const override;
    virtual Node child(Node const*, size_t idx) const override;

    Db::Core::Database const& m_db;
    std::vector<std::pair<std::string, Db::Core::Table const*>> m_tables;
};

}
