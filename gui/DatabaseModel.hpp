#pragma once

#include "gui/Structure.hpp"
#include "gui/client/DatabaseClient.hpp"
#include <Essa/GUI/Model.hpp>

namespace EssaDB {

class DatabaseModel : public GUI::Model {
public:
    explicit DatabaseModel() { }

    Db::Core::DbErrorOr<void> update(DatabaseClient const*);

private:
    virtual size_t column_count() const override;
    virtual GUI::ModelColumn column(size_t column) const override;
    virtual GUI::Variant data(Node const&, size_t column) const override;
    virtual llgl::Texture const* icon(Node const&) const override;
    virtual size_t children_count(Node const*) const override;
    virtual Node child(Node const*, size_t idx) const override;

    Structure::Database m_structure;
};

}
