# Code structure

The EssaDB root directory is divided into various subdirectories containing different components of the software.

## `assets`

Resources required by GUI (textures and fonts)

## `db`

Code for the EssaDB engine itself - runtime and SQL parser.

### `db/core`

The runtime. Contains abstractions for databases, tables, rows, columns, values etc. Handles actual running of SQL code.

### `db/sql`

The SQL parser and lexer, along with some high-level functions for running SQL queries. Depends on `core`.

### `db/storage`

Code for reading and writing EssaDB databases to files. Depends on `core`.

## `docs`

Documentation.

## `gui`

Code for the EssaDB GUI - MS Access-like graphical frontend for EssaDB.

## `repl`

Code for the EssaDB REPL - the interactive (Essa)SQL console like `mysql`.

## `tests`

Unit tests for EssaDB.

### `tests/sql`

Contains SQL files to be tested along with expected output/error.

### `tests/testcases`

Contains harness for running SQL and some test that are not yet ported to SQL or cannot be run yet in SQL (e.g [tuple comparison](/tests/testcases/arithmetic.cpp))
