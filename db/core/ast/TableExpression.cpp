#include "TableExpression.hpp"

#include <db/core/Database.hpp>

namespace Db::Core::AST {

Tuple TableExpression::create_joined_tuple(const Tuple& lhs_row, const Tuple& rhs_row) {
    std::vector<Value> row;
    for (size_t i = 0; i < lhs_row.value_count(); i++) {
        row.push_back(lhs_row.value(i));
    }
    for (size_t i = 0; i < rhs_row.value_count(); i++) {
        row.push_back(rhs_row.value(i));
    }
    return Tuple(row);
}

DbErrorOr<std::unique_ptr<Table>> SimpleTableExpression::evaluate(EvaluationContext&) const {
    // FIXME: This totally does not copy the entire table
    //        just to "evaluate" the identifier.
    MemoryBackedTable table(nullptr, m_table.name());

    for (const auto& column : m_table.columns()) {
        TRY(table.add_column(column));
    }

    for (const auto& row : m_table.raw_rows()) {
        TRY(table.insert(row));
    }

    return std::make_unique<MemoryBackedTable>(std::move(table));
}

std::string SimpleTableExpression::to_string() const {
    return "<SimpleTableExpression>";
}

DbErrorOr<std::optional<size_t>> SimpleTableExpression::resolve_identifier(Database*, Identifier const& id) const {
    auto column = m_table.get_column(id.id());
    if (!column) {
        return DbError { "Column '" + id.id() + "' does not exist in table '" + m_table.name() + "'", start() };
    }
    return column->index;
}

DbErrorOr<size_t> SimpleTableExpression::column_count(Database*) const {
    return m_table.columns().size();
}

DbErrorOr<std::unique_ptr<Table>> TableIdentifier::evaluate(EvaluationContext& context) const {
    if (!context.db) {
        // FIXME: The message should mention calling USE db; when this is implemented.
        return DbError { "Cannot evaluate table identifier without database", start() };
    }

    // FIXME: Yes, this abstraction is entirely wrong...
    auto table_ptr = TRY(context.db->table(m_id));
    MemoryBackedTable table(nullptr, table_ptr->name());

    for (const auto& column : table_ptr->columns()) {
        TRY(table.add_column(column));
    }

    for (const auto& row : table_ptr->raw_rows()) {
        TRY(table.insert(row));
    }

    return std::make_unique<MemoryBackedTable>(std::move(table));
}

DbErrorOr<std::optional<size_t>> TableIdentifier::resolve_identifier(Database* db, Identifier const& id) const {
    if (!db) {
        return DbError { "Table identifiers cannot be resolved without a database", start() };
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
        return DbError { "Column '" + id.id() + "' does not exist in table '" + m_id + "'", id.start() };
    }
    return column->index;
}

DbErrorOr<size_t> TableIdentifier::column_count(Database* db) const {
    if (!db) {
        return DbError { "Table identifiers cannot be resolved without a database", start() };
    }
    auto table = TRY(db->table(m_id));
    return table->columns().size();
}

DbErrorOr<std::unique_ptr<Table>> JoinExpression::evaluate(EvaluationContext& context) const {
    auto table = std::make_unique<MemoryBackedTable>(nullptr, "");

    auto lhs_ptr = TRY(m_lhs->evaluate(context));
    auto rhs_ptr = TRY(m_rhs->evaluate(context));

    auto lhs = lhs_ptr.get();
    auto rhs = rhs_ptr.get();

    std::multimap<Value, std::pair<Table*, Tuple>, ValueSorter> contents;

    auto add_columns = [table = table.get(), &contents](Table* source_table, Identifier const* on_id) -> DbErrorOr<void> {
        bool add_to_index = true;
        size_t index = 0;
        for (const auto& column : source_table->columns()) {
            if (column.name() == on_id->referenced_columns().front())
                add_to_index = false;
            if (add_to_index)
                index++;
            TRY(table->add_column(Column(column.name(), column.type(), false, false, false)));
        }
        for (auto const& row : source_table->raw_rows()) {
            if (index >= row.value_count()) {
                return DbError { fmt::format("Invalid column `{}` used in join expression", on_id->to_string()), 0 };
            }
            contents.insert(std::pair(row.value(index), std::make_pair(source_table, row)));
        }
        return {};
    };

    TRY(add_columns(lhs, m_on_lhs.get()));
    TRY(add_columns(rhs, m_on_rhs.get()));

    auto beg = contents.begin();
    beg++;
    auto last = contents.begin();

    switch (m_join_type) {
    case Type::InnerJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs && it->second.first == rhs) {
                if (TRY(it->first == last->first)) {
                    auto row = create_joined_tuple(last->second.second, it->second.second);
                    TRY(table->insert(row));
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
            if (last->second.first == lhs) {
                if (TRY(it->first == last->first)) {
                    auto row = create_joined_tuple(last->second.second, it->second.second);
                    TRY(table->insert(row));
                    it++;
                    if (it == contents.end())
                        break;
                }
                else {
                    std::vector<Value> values(rhs->columns().size(), Value::null());
                    Tuple dummy(values);

                    auto row = create_joined_tuple(last->second.second, dummy);

                    TRY(table->insert(row));
                }
            }
            last = it;
        }

        break;
    }

    case Type::RightJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == rhs || it->second.first == rhs) {
                if (TRY(it->first == last->first)) {
                    auto row = create_joined_tuple(last->second.second, it->second.second);
                    TRY(table->insert(row));
                    it++;
                    if (it == contents.end())
                        break;

                    last = it;
                }
                else {
                    std::vector<Value> values(lhs->columns().size(), Value::null());
                    Tuple dummy(values);

                    auto row = create_joined_tuple(dummy, it->second.second);

                    TRY(table->insert(row));
                }
            }
            last = it;
        }

        break;
    }

    case Type::OuterJoin: {
        for (auto it = beg; it != contents.end(); it++) {
            if (last->second.first == lhs && it->second.first == rhs && TRY(it->first == last->first)) {
                auto row = create_joined_tuple(last->second.second, it->second.second);
                TRY(table->insert(row));
                it++;
                if (it == contents.end())
                    break;
            }
            else if (last->second.first == lhs) {
                std::vector<Value> values(rhs->columns().size(), Value::null());
                Tuple dummy(values);

                auto row = create_joined_tuple(last->second.second, dummy);

                TRY(table->insert(row));
            }
            else if (last->second.first == rhs) {
                std::vector<Value> values(lhs->columns().size(), Value::null());
                Tuple dummy(values);

                auto row = create_joined_tuple(dummy, it->second.second);

                TRY(table->insert(row));
            }
            last = it;
        }

        break;
    }

    case Type::Invalid:
        return DbError { fmt::format("Internal error: Invalid join type"), start() };
        break;
    }

    return table;
}

DbErrorOr<std::optional<size_t>> JoinExpression::resolve_identifier(Database* db, Identifier const& id) const {
    auto lhs_id = TRY(m_lhs->resolve_identifier(db, id));
    if (lhs_id)
        return lhs_id;
    auto rhs_id = TRY(m_rhs->resolve_identifier(db, id));
    if (!rhs_id)
        return rhs_id;
    return *rhs_id + TRY(m_lhs->column_count(db));
}

DbErrorOr<size_t> JoinExpression::column_count(Database* db) const {
    return TRY(m_lhs->column_count(db)) + TRY(m_rhs->column_count(db));
}

DbErrorOr<std::unique_ptr<Table>> CrossJoinExpression::evaluate(EvaluationContext& context) const {
    auto table = std::make_unique<MemoryBackedTable>(nullptr, "");

    auto lhs_ptr = TRY(m_lhs->evaluate(context));
    auto rhs_ptr = TRY(m_rhs->evaluate(context));

    auto lhs = dynamic_cast<MemoryBackedTable*>(lhs_ptr.get());
    auto rhs = dynamic_cast<MemoryBackedTable*>(rhs_ptr.get());

    for (const auto& column : lhs->columns()) {
        TRY(table->add_column(Column(column.name(), column.type(), false, false, false)));
    }

    for (const auto& column : rhs->columns()) {
        TRY(table->add_column(Column(column.name(), column.type(), false, false, false)));
    }

    for (const auto& lhs_row : lhs->raw_rows()) {
        for (const auto& rhs_row : rhs->raw_rows()) {
            auto row = create_joined_tuple(lhs_row, rhs_row);
            TRY(table->insert(row));
        }
    }

    return table;
}

DbErrorOr<std::optional<size_t>> CrossJoinExpression::resolve_identifier(Database* db, Identifier const& id) const {
    auto lhs_id = TRY(m_lhs->resolve_identifier(db, id));
    if (lhs_id)
        return lhs_id;
    auto rhs_id = TRY(m_rhs->resolve_identifier(db, id));
    if (!rhs_id)
        return rhs_id;
    return *rhs_id + TRY(m_lhs->column_count(db));
}

DbErrorOr<size_t> CrossJoinExpression::column_count(Database* db) const {
    return TRY(m_lhs->column_count(db)) + TRY(m_rhs->column_count(db));
}
}
