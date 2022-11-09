#include "db/core/IndexedRelation.hpp"
#include "db/core/Table.hpp"
#include <EssaUtil/Config.hpp>
#include <db/core/ast/Statement.hpp>

#include <EssaUtil/ScopeGuard.hpp>
#include <db/core/Database.hpp>
#include <db/core/ast/EvaluationContext.hpp>
#include <db/core/ast/TableExpression.hpp>

namespace Db::Core::AST {

DbErrorOr<ValueOrResultSet> DeleteFrom::execute(Database& db) const {
    auto table = static_cast<MemoryBackedTable*>(TRY(db.table(m_from)));

    EvaluationContext context { .db = &db };
    AST::SimpleTableExpression id { 0, *table };
    SelectColumns columns;
    context.frames.emplace_back(&id, columns);

    auto should_include_row = [&](Tuple const& row) -> DbErrorOr<bool> {
        if (!m_where)
            return true;
        context.current_frame().row = { .tuple = row, .source = {} };
        return TRY(m_where->evaluate(context)).to_bool();
    };

    // TODO: Maintain integrity on error
    std::optional<DbError> error;
    std::vector<size_t> rows_to_remove;

    size_t idx = 0;
    TRY(table->rows().try_for_each_row([&](auto const& row) -> DbErrorOr<void> {
        Util::ScopeGuard guard {
            [&idx] { idx++; }
        };
        auto should_include = TRY(should_include_row(row));
        if (should_include) {
            rows_to_remove.push_back(idx);
        }
        return {};
    }));

    for (auto it = rows_to_remove.rbegin(); it != rows_to_remove.rend(); it++) {
        table->raw_rows().erase(table->raw_rows().begin() + *it);
    }
    return Value::null();
}

DbErrorOr<ValueOrResultSet> Update::execute(Database& db) const {
    auto table = static_cast<MemoryBackedTable*>(TRY(db.table(m_table)));
    EvaluationContext context { .db = &db };
    AST::SimpleTableExpression id { 0, *table };
    SelectColumns columns;
    context.frames.emplace_back(&id, columns);

    for (const auto& update_pair : m_to_update) {
        auto column = table->get_column(update_pair.column);

        for (auto& tuple : table->raw_rows()) {
            context.current_frame().row = { .tuple = tuple, .source = {} };
            tuple.set_value(column->index, TRY(update_pair.expr->evaluate(context)));
        }
    }

    return Value::null();
}

DbErrorOr<ValueOrResultSet> CreateTable::execute(Database& db) const {
    auto& table = db.create_table(m_name, m_check);
    for (auto const& column : m_columns) {
        TRY(table.add_column(column.column));
        std::visit(
            Util::Overloaded {
                [](std::monostate) {},
                [&](PrimaryKey const& pk) {
                    table.set_primary_key(pk);
                },
                [&](ForeignKey const& fk) {
                    table.add_foreign_key(fk);
                },
            },
            column.key);
    }
    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> DropTable::execute(Database& db) const {
    TRY(db.drop_table(m_name));

    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> TruncateTable::execute(Database& db) const {
    auto table = TRY(db.table(m_name));
    TRY(table->truncate());

    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> AlterTable::execute(Database& db) const {
    auto table = TRY(db.table(m_name));

    for (const auto& to_add : m_to_add) {
        TRY(table->add_column(to_add.column));
        std::visit(
            Util::Overloaded {
                [](std::monostate) {},
                [&](PrimaryKey const& pk) {
                    table->set_primary_key(pk);
                },
                [&](ForeignKey const& fk) {
                    table->add_foreign_key(fk);
                },
            },
            to_add.key);
    }

    for (const auto& to_alter : m_to_alter) {
        TRY(table->alter_column(to_alter.column));
        std::visit(
            Util::Overloaded {
                [](std::monostate) {},
                [&](PrimaryKey const& pk) {
                    table->set_primary_key(pk);
                },
                [&](ForeignKey const& fk) {
                    table->add_foreign_key(fk);
                },
            },
            to_alter.key);
    }

    for (const auto& to_drop : m_to_drop) {
        TRY(table->drop_column(to_drop));
        if (table->primary_key() && table->primary_key()->local_column == to_drop) {
            table->set_primary_key({});
        }
        table->drop_foreign_key(to_drop);
    }

    auto memory_backed_table = dynamic_cast<MemoryBackedTable*>(table);

    if (!memory_backed_table)
        return DbError { "Table with invalid type!", 0 };

    auto check_exists = [&]() -> DbErrorOr<bool> {
        if (memory_backed_table->check())
            return true;
        return DbError { "No check to alter!", 0 };
    };

    if (m_check_to_add && TRY(check_exists()))
        TRY(memory_backed_table->check()->add_check(m_check_to_add));

    if (m_check_to_alter && TRY(check_exists()))
        TRY(memory_backed_table->check()->alter_check(m_check_to_alter));

    if (m_check_to_drop && TRY(check_exists()))
        TRY(memory_backed_table->check()->drop_check());

    for (const auto& to_add : m_constraint_to_add) {
        if (TRY(check_exists()))
            TRY(memory_backed_table->check()->add_constraint(to_add.first, to_add.second));
    }

    for (const auto& to_alter : m_constraint_to_alter) {
        if (TRY(check_exists()))
            TRY(memory_backed_table->check()->alter_constraint(to_alter.first, to_alter.second));
    }

    for (const auto& to_drop : m_constraint_to_drop) {
        if (TRY(check_exists()))
            TRY(memory_backed_table->check()->drop_constraint(to_drop));
    }

    return { Value::null() };
}

DbErrorOr<ValueOrResultSet> Import::execute(Database& db) const {
    TRY(db.import_to_table(m_filename, m_table, m_mode));
    return Value::null();
}

}
