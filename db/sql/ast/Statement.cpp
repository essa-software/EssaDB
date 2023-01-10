#include "db/core/Relation.hpp"
#include "db/core/TableSetup.hpp"
#include "db/sql/SQLError.hpp"
#include <db/sql/ast/Statement.hpp>

#include <EssaUtil/Config.hpp>
#include <EssaUtil/ScopeGuard.hpp>
#include <db/core/Database.hpp>
#include <db/core/DbError.hpp>
#include <db/core/IndexedRelation.hpp>
#include <db/core/Table.hpp>
#include <db/core/ValueOrResultSet.hpp>
#include <db/sql/ast/EvaluationContext.hpp>
#include <db/sql/ast/TableExpression.hpp>
#include <iostream>
#include <string>

namespace Db::Sql::AST {

SQLErrorOr<Core::ValueOrResultSet> StatementList::execute(Core::Database& db) const {
    if (m_statements.empty()) {
        return SQLError { "Empty statement list", 0 };
    }

    std::optional<Core::ValueOrResultSet> result;
    for (auto const& stmt : m_statements) {
        TRY(stmt->execute(db));
    }
    return *result;
}

SQLErrorOr<Core::ValueOrResultSet> DeleteFrom::execute(Core::Database& db) const {
    auto table = TRY(db.table(m_from).map_error(DbToSQLError { start() }));
    auto memory_backed_table = dynamic_cast<Core::MemoryBackedTable*>(table);
    if (!memory_backed_table) {
        return SQLError { "TODO: Support other table classes than MemoryBackedTable", start() };
    }

    EvaluationContext context { .db = &db };
    AST::SimpleTableExpression id { 0, *table };
    SelectColumns columns;
    context.frames.emplace_back(&id, columns);

    auto should_include_row = [&](Core::Tuple const& row) -> SQLErrorOr<bool> {
        if (!m_where)
            return true;
        context.current_frame().row = { .tuple = row, .source = {} };
        return TRY(m_where->evaluate(context)).to_bool().map_error(DbToSQLError { start() });
    };

    // TODO: Maintain integrity on error
    std::optional<Core::DbError> error;
    std::vector<size_t> rows_to_remove;

    size_t idx = 0;
    TRY(table->rows().try_for_each_row([&](auto const& row) -> SQLErrorOr<void> {
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
        memory_backed_table->raw_rows().erase(memory_backed_table->raw_rows().begin() + *it);
    }
    return Core::Value::null();
}

SQLErrorOr<Core::ValueOrResultSet> Update::execute(Core::Database& db) const {
    auto table = TRY(db.table(m_table).map_error(DbToSQLError { start() }));

    EvaluationContext context { .db = &db };
    AST::SimpleTableExpression id { 0, *table };
    SelectColumns columns;
    context.frames.emplace_back(&id, columns);

    // FIXME: Integrity
    // FIXME: This can be optimized to write for every column, not for every written value.
    for (const auto& update_pair : m_to_update) {
        auto column = table->get_column(update_pair.column);

        TRY(table->writable_rows().try_for_each_row_reference([&](Core::RowReference& row) -> SQLErrorOr<void> {
            auto tuple = row.read();
            context.current_frame().row = { .tuple = tuple, .source = {} };
            tuple.set_value(column->index, TRY(update_pair.expr->evaluate(context)));
            row.write(tuple);
            return {};
        }));
    }

    return Core::Value::null();
}

bool TableStatement::table_exists(Core::Database& db, std::string const& name) const {
    if (m_existence == ExistenceCondition::UNSPECIFIED) {
        return true;
    }

    if (m_existence == ExistenceCondition::EXISTS) {
        return db.exists(name);
    }
    else {
        return !db.exists(name);
    }
}

SQLErrorOr<Core::ValueOrResultSet> CreateTable::execute(Core::Database& db) const {
    if (!table_exists(db, m_name)) {
        return { Core::Value::null() };
    }

    Core::TableSetup setup;
    setup.name = m_name;
    for (auto const& column : m_columns) {
        setup.columns.push_back(column.column);
    }
    auto table = TRY(db.create_table(std::move(setup), m_check, m_engine.value_or(db.default_engine())).map_error(DbToSQLError { start() }));

    for (auto const& column : m_columns) {
        std::visit(
            Util::Overloaded {
                [](std::monostate) {},
                [&](Core::PrimaryKey const& pk) {
                    table->set_primary_key(pk);
                },
                [&](Core::ForeignKey const& fk) {
                    table->add_foreign_key(fk);
                },
            },
            column.key);
    }
    return { Core::Value::null() };
}

SQLErrorOr<Core::ValueOrResultSet> DropTable::execute(Core::Database& db) const {
    if (!table_exists(db, m_name)) {
        return { Core::Value::null() };
    }

    TRY(db.drop_table(m_name).map_error(DbToSQLError { start() }));

    return { Core::Value::null() };
}

SQLErrorOr<Core::ValueOrResultSet> TruncateTable::execute(Core::Database& db) const {
    if (!table_exists(db, m_name)) {
        return { Core::Value::null() };
    }

    // Just drop table and recreate it with the same settings :^)
    // FIXME: Handle primary and foreign keys
    auto table = TRY(db.table(m_name).map_error(DbToSQLError { start() }));
    Core::TableSetup setup { table->name(), table->columns() };
    auto memory_backed_table = dynamic_cast<Core::MemoryBackedTable*>(table);
    auto check = memory_backed_table ? memory_backed_table->check() : nullptr;
    auto engine = table->engine();

    TRY(db.drop_table(m_name).map_error(DbToSQLError { start() }));
    TRY(db.create_table(setup, check, engine).map_error(DbToSQLError { start() }));
    return { Core::Value::null() };
}

SQLErrorOr<Core::ValueOrResultSet> AlterTable::execute(Core::Database& db) const {
    if (!table_exists(db, m_name)) {
        return { Core::Value::null() };
    }

    auto table = TRY(db.table(m_name).map_error(DbToSQLError { start() }));

    std::vector<Core::Column> new_columns = table->columns();

    for (const auto& to_add : m_to_add) {
        new_columns.push_back(to_add.column);
        std::visit(
            Util::Overloaded {
                [](std::monostate) {},
                [&](Core::PrimaryKey const& pk) {
                    table->set_primary_key(pk);
                },
                [&](Core::ForeignKey const& fk) {
                    table->add_foreign_key(fk);
                },
            },
            to_add.key);
    }

    for (const auto& to_alter : m_to_alter) {
        for (size_t i = 0; i < new_columns.size(); i++) {
            if (new_columns[i].name() == to_alter.column.name()) {
                new_columns[i] = std::move(to_alter.column);
            }
        }
        std::visit(
            Util::Overloaded {
                [](std::monostate) {},
                [&](Core::PrimaryKey const& pk) {
                    table->set_primary_key(pk);
                },
                [&](Core::ForeignKey const& fk) {
                    table->add_foreign_key(fk);
                },
            },
            to_alter.key);
    }

    for (const auto& to_drop : m_to_drop) {
        std::vector<Core::Column> vec;
        for (size_t i = 0; i < new_columns.size(); i++) {
            if (new_columns[i].name() != to_drop) {
                vec.push_back(std::move(new_columns[i]));
            }
        }
        new_columns = std::move(vec);

        if (table->primary_key() && table->primary_key()->local_column == to_drop) {
            table->set_primary_key({});
        }
        table->drop_foreign_key(to_drop);
    }

    TRY(db.restructure_table(m_name, { m_name, new_columns }).map_error(DbToSQLError { start() }));
    table = TRY(db.table(m_name).map_error(DbToSQLError { start() }));

    auto memory_backed_table = dynamic_cast<Core::MemoryBackedTable*>(table);
    auto validate_check_exists_and_is_supported = [&]() -> SQLErrorOr<void> {
        if (!memory_backed_table) {
            return SQLError { "(FIXME) Checks are supported only on MemoryBackedTables", start() };
        }
        if (memory_backed_table->check())
            return {};
        return SQLError { "No check to alter", start() };
    };

    if (m_check_to_add) {
        TRY(validate_check_exists_and_is_supported());
        TRY(memory_backed_table->check()->add_check(m_check_to_add));
    }

    if (m_check_to_alter) {
        TRY(validate_check_exists_and_is_supported());
        TRY(memory_backed_table->check()->alter_check(m_check_to_alter));
    }

    if (m_check_to_drop) {
        TRY(validate_check_exists_and_is_supported());
        TRY(memory_backed_table->check()->drop_check());
    }

    for (const auto& to_add : m_constraint_to_add) {
        TRY(validate_check_exists_and_is_supported());
        TRY(memory_backed_table->check()->add_constraint(to_add.first, to_add.second));
    }

    for (const auto& to_alter : m_constraint_to_alter) {
        TRY(validate_check_exists_and_is_supported());
        TRY(memory_backed_table->check()->alter_constraint(to_alter.first, to_alter.second));
    }

    for (const auto& to_drop : m_constraint_to_drop) {
        TRY(validate_check_exists_and_is_supported());
        TRY(memory_backed_table->check()->drop_constraint(to_drop));
    }

    return { Core::Value::null() };
}

SQLErrorOr<Core::ValueOrResultSet> Import::execute(Core::Database& db) const {
    TRY(db.import_to_table(m_filename, m_table, m_mode, m_engine.value_or(db.default_engine())).map_error(DbToSQLError { start() }));
    return Core::Value::null();
}

}
