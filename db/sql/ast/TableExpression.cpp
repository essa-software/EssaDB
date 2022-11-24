#include "TableExpression.hpp"

#include <EssaUtil/Config.hpp>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>

namespace Db::Sql::AST {

Core::Tuple TableExpression::create_joined_tuple(const Core::Tuple& lhs_row, const Core::Tuple& rhs_row) {
    std::vector<Core::Value> row;
    for (size_t i = 0; i < lhs_row.value_count(); i++) {
        row.push_back(lhs_row.value(i));
    }
    for (size_t i = 0; i < rhs_row.value_count(); i++) {
        row.push_back(rhs_row.value(i));
    }
    return Core::Tuple(row);
}

class NonOwningTableWrapper : public Core::Relation {
public:
    NonOwningTableWrapper(Core::Relation const& other)
        : m_other(other) { }

    virtual std::vector<Core::Column> const& columns() const { return m_other.columns(); }
    virtual Core::RelationIterator rows() const { return m_other.rows(); }
    virtual size_t size() const { return m_other.size(); }

private:
    Core::Relation const& m_other;
};

Core::DbErrorOr<std::unique_ptr<Core::Relation>> SimpleTableExpression::evaluate(EvaluationContext&) const {
    return std::make_unique<NonOwningTableWrapper>(m_table);
}

std::string SimpleTableExpression::to_string() const {
    return "<SimpleTableExpression>";
}

Core::DbErrorOr<std::optional<size_t>> SimpleTableExpression::resolve_identifier(Core::Database*, Identifier const& id) const {
    auto column = m_table.get_column(id.id());
    if (!column) {
        return Core::DbError { "Column '" + id.id() + "' does not exist in table '" + m_table.name() + "'", start() };
    }
    return column->index;
}

Core::DbErrorOr<size_t> SimpleTableExpression::column_count(Core::Database*) const {
    return m_table.columns().size();
}

Core::DbErrorOr<std::unique_ptr<Core::Relation>> TableIdentifier::evaluate(EvaluationContext& context) const {
    if (!context.db) {
        // FIXME: The message should mention calling USE db; when this is implemented.
        return Core::DbError { "Cannot evaluate table identifier without database", start() };
    }

    return std::make_unique<NonOwningTableWrapper>(*TRY(context.db->table(m_id)));
}

Core::DbErrorOr<std::optional<size_t>> TableIdentifier::resolve_identifier(Core::Database* db, Identifier const& id) const {
    if (!db) {
        return Core::DbError { "Table identifiers cannot be resolved without a database", start() };
    }
    auto requested_table_name = id.table().has_value() ? id.table().value() : m_id;
    if (requested_table_name != m_id && requested_table_name != m_alias) {
        return std::optional<size_t> {};
    }
    auto maybe_table = db->table(m_id);
    if (maybe_table.is_error()) {
        return std::optional<size_t> {};
    }
    auto table = maybe_table.release_value();
    auto column = table->get_column(id.id());
    if (!column) {
        // TODO: Location info
        return Core::DbError { "Column '" + id.id() + "' does not exist in table '" + m_id + "'", id.start() };
    }
    return column->index;
}

Core::DbErrorOr<size_t> TableIdentifier::column_count(Core::Database* db) const {
    if (!db) {
        return Core::DbError { "Table identifiers cannot be resolved without a database", start() };
    }
    auto table = TRY(db->table(m_id));
    return table->columns().size();
}

Core::DbErrorOr<std::unique_ptr<Core::Relation>> JoinExpression::evaluate(EvaluationContext& context) const {
    auto table = std::make_unique<Core::MemoryBackedTable>(nullptr, "");

    auto lhs = TRY(m_lhs->evaluate(context));
    auto rhs = TRY(m_rhs->evaluate(context));

    std::multimap<Core::Value, std::pair<Core::Relation*, Core::Tuple>, Core::ValueSorter> contents;

    auto add_columns = [table = table.get(), &contents](Core::Relation& source_table, Identifier const* on_id) -> Core::DbErrorOr<void> {
        bool add_to_index = true;
        size_t index = 0;
        for (const auto& column : source_table.columns()) {
            if (column.name() == on_id->referenced_columns().front())
                add_to_index = false;
            if (add_to_index)
                index++;
            TRY(table->add_column(Core::Column(column.name(), column.type(), false, false, false)));
        }
        TRY(source_table.rows().try_for_each_row([&](auto const& row) -> Core::DbErrorOr<void> {
            if (index >= row.value_count()) {
                return Core::DbError { fmt::format("Invalid column `{}` used in join expression", on_id->to_string()), 0 };
            }
            contents.insert(std::pair(row.value(index), std::make_pair(&source_table, row)));
            return {};
        }));
        return {};
    };

    TRY(add_columns(*lhs, m_on_lhs.get()));
    TRY(add_columns(*rhs, m_on_rhs.get()));

    auto beg = contents.begin();
    beg++;
    auto last = contents.begin();

    switch (m_join_type) {
    case Type::InnerJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs.get() && it->second.first == rhs.get()) {
                if (TRY(it->first == last->first)) {
                    auto row = create_joined_tuple(last->second.second, it->second.second);
                    TRY(table->insert_unchecked(row));
                    it++;
                    if (it == contents.end())
                        break;
                }
            }
            last = it;
        }

        break;
    }

    case Type::LeftJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs.get()) {
                if (TRY(it->first == last->first)) {
                    auto row = create_joined_tuple(last->second.second, it->second.second);
                    TRY(table->insert_unchecked(row));
                    it++;
                    if (it == contents.end())
                        break;
                }
                else {
                    std::vector<Core::Value> values(rhs->columns().size(), Core::Value::null());
                    Core::Tuple dummy(values);

                    auto row = create_joined_tuple(last->second.second, dummy);

                    TRY(table->insert_unchecked(row));
                }
            }
            last = it;
        }

        break;
    }

    case Type::RightJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == rhs.get() || it->second.first == rhs.get()) {
                if (TRY(it->first == last->first)) {
                    auto row = create_joined_tuple(last->second.second, it->second.second);
                    TRY(table->insert_unchecked(row));
                    it++;
                    if (it == contents.end())
                        break;

                    last = it;
                }
                else {
                    std::vector<Core::Value> values(lhs->columns().size(), Core::Value::null());
                    Core::Tuple dummy(values);

                    auto row = create_joined_tuple(dummy, it->second.second);

                    TRY(table->insert_unchecked(row));
                }
            }
            last = it;
        }

        break;
    }

    case Type::OuterJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs.get() && it->second.first == rhs.get() && TRY(it->first == last->first)) {
                auto row = create_joined_tuple(last->second.second, it->second.second);
                TRY(table->insert_unchecked(row));
                it++;
                if (it == contents.end())
                    break;
            }
            else if (last->second.first == lhs.get()) {
                std::vector<Core::Value> values(rhs->columns().size(), Core::Value::null());
                Core::Tuple dummy(values);

                auto row = create_joined_tuple(last->second.second, dummy);

                TRY(table->insert_unchecked(row));
            }
            else if (last->second.first == rhs.get()) {
                std::vector<Core::Value> values(lhs->columns().size(), Core::Value::null());
                Core::Tuple dummy(values);

                auto row = create_joined_tuple(dummy, it->second.second);

                TRY(table->insert_unchecked(row));
            }
            last = it;
        }

        break;
    }

    case Type::Invalid:
        return Core::DbError { fmt::format("Internal error: Invalid join type"), start() };
        break;
    }

    return table;
}

Core::DbErrorOr<std::optional<size_t>> JoinExpression::resolve_identifier(Core::Database* db, Identifier const& id) const {
    auto lhs_id = TRY(m_lhs->resolve_identifier(db, id));
    if (lhs_id)
        return lhs_id;
    auto rhs_id = TRY(m_rhs->resolve_identifier(db, id));
    if (!rhs_id)
        return rhs_id;
    return *rhs_id + TRY(m_lhs->column_count(db));
}

Core::DbErrorOr<size_t> JoinExpression::column_count(Core::Database* db) const {
    return TRY(m_lhs->column_count(db)) + TRY(m_rhs->column_count(db));
}

Core::DbErrorOr<std::unique_ptr<Core::Relation>> CrossJoinExpression::evaluate(EvaluationContext& context) const {
    auto table = std::make_unique<Core::MemoryBackedTable>(nullptr, "");

    auto lhs = TRY(m_lhs->evaluate(context));
    auto rhs = TRY(m_rhs->evaluate(context));

    for (const auto& column : lhs->columns()) {
        TRY(table->add_column(Core::Column(column.name(), column.type(), false, false, false)));
    }

    for (const auto& column : rhs->columns()) {
        TRY(table->add_column(Core::Column(column.name(), column.type(), false, false, false)));
    }

    TRY(lhs->rows().try_for_each_row([&](auto const& lhs_row) -> Core::DbErrorOr<void> {
        TRY(rhs->rows().try_for_each_row([&](auto const& rhs_row) -> Core::DbErrorOr<void> {
            auto row = create_joined_tuple(lhs_row, rhs_row);
            TRY(table->insert_unchecked(row));
            return {};
        }));
        return {};
    }));

    return table;
}

Core::DbErrorOr<std::optional<size_t>> CrossJoinExpression::resolve_identifier(Core::Database* db, Identifier const& id) const {
    auto lhs_id = TRY(m_lhs->resolve_identifier(db, id));
    if (lhs_id)
        return lhs_id;
    auto rhs_id = TRY(m_rhs->resolve_identifier(db, id));
    if (!rhs_id)
        return rhs_id;
    return *rhs_id + TRY(m_lhs->column_count(db));
}

Core::DbErrorOr<size_t> CrossJoinExpression::column_count(Core::Database* db) const {
    return TRY(m_lhs->column_count(db)) + TRY(m_rhs->column_count(db));
}
}
