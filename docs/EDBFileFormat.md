# EDB file format

This file describes file format used for storing EssaDB databases.

## Overview
A database storage consists of a single directory. The directory contains global database file (`db.ini`) and a separate file for every table (`<table name>.edb`).
Every table is stored in a separate file. This file consists of header and a heap, which stores actual data.

## Primitive data types

* `u8/u16/u32/u64 XX` - unsigned integers of endianness `XX` (`LE` - little endian)
* `i8/i16/i32/i64 XX` - 2's complement signed integers of endianness `XX` (`LE` - little endian)
* `f32/f64` - [IEEE 754](https://standards.ieee.org/ieee/754/6210/) binary32 and binary64 (little-endian), respectively
* `bool` - Boolean, written as `u8` where `0` is false and `1` is true.

## General table file layout

### Header
Header structure (packed)

```c++
struct EDBHeader {
    u8 magic[6];                   // Filemagic (`esdb\r\n` / `65 73 64 62 0d 0a`).
    u16le version;                 // File version. This document describes version `0x0000`.

    u32le block_size;              // Block size

    u64le row_count;               // Number of rows
    u8 column_count;               // Number of columns

    HeapPtr first_row_ptr;         // Pointer to first row (MUST be null if table is empty)
    HeapPtr last_row_ptr;          // Pointer to last row (MUST be null if table is empty)
    BlockIndex last_table_block;   // Index of last table block
    BlockIndex last_heap_block;    // Index of last heap block
    
    HeapSpan table_name;           // Pointer to table name
    HeapSpan check_statement;      // Pointer to check statement (stored as SQL expression)

    u8 auto_increment_value_count; // Number of auto-increment variables
    u8 key_count;                  // Number of keys

    Col columns[column_count];     // Column definitions
    Aiv ai_values[ai_value_count]; // Auto-increment variable definitions
    Key keys[key_count];           // Key definitions
}
```

Header size can be calculated using following formula:

sizeof(`EDBHeader`) + sizeof(`Col`) * `column_count` + sizeof(`Aiv`) * `auto_increment_value_count` + sizeof(`Key`) * `key_count`

#### Column format (`Col`):

| Size (B)  | Offset (B)    | Type                              | Usage
|-          |-              |-                                  |-
| 14        | 0             | `HeapSpan`                        | Column name
| 1         | 14            | [`ValueType`](#valuetype)         | Value type
| 1         | 15            | `bool`                            | Auto increment
| 1         | 16            | `bool`                            | Is unique
| 1         | 17            | `bool`                            | Is non null
| 1         | 18            | `bool`                            | Has default value
| 14        | 19            | [`Value`](#value)                 | Default value

#### Auto-increment variable format (`Aiv`):

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 1         | 0             | `u8`          | Column index
| 8         | 1             | `u64 LE`      | Value

#### Key format (`Key`):

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 1         | 0             | `KeyType`     | Key type
| 1         | 1             | `u8`          | Local column
| 14        | 2             | `HeapSpan`    | References table (for FOREIGN KEY)
| 1         | 16            | `u8`          | References column (for FOREIGN KEY)

#### Key types:
* `0x00` - PRIMARY
* `0x01` - FOREIGN

### Heap
Directly after headers there is a *heap*. Heap is structured in a two-tier way:
* Tier 1 - blocks. There are 3 kinds of blocks:
    * `Table` - stores rows.
    * `Heap` - stores small dynamic data, like varchars, blobs and other strings.
    * `Big` - stores data that don't fit in small blocks, such as big blobs.
* Tier 2:
    * for `Table` blocks, a linked list of rows
    * for `Heap` blocks, [free store](https://github.com/sppmacd/heap/blob/master/heap.cpp). It is implemented using linked list like structure.
    * for `Big` blocks, just data.

Blocks are sized so that they fits a header + 255 rows. The total block size is stored in *block size* field of the main header.

Two first blocks are reserved: BlockIndex `1` for first Table block, BlockIndex `2` for first Heap block.

Every block contains a header:
| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 1         | 0             | `u8`          | Block type: 0 - free block, 1 - `Table`, 2 - `Heap`, 3 - `Big`
| 4         | 1             | `BlockIndex`  | Prev block index (0 if none)
| 4         | 5             | `BlockIndex`  | Next block index (0 if none)

Directly after header come data.

**Block** is adressed by `BlockIndex`, which is a `u32 LE`.

**Pointer into heap** is specified as `HeapPtr` (a `BlockIndex` + `u32 LE` offset in block, relative to block beginning), 8 B total.

**Contiguous data span** is specified by `HeapSpan`, which consists of `HeapPtr` and a `u64 LE` specifying size, 16 B total.

## Region types

### Table

`Table` contains actual rows, grouped into chunks of 255. It allows for reasonably fast searching for free space to insert new blocks. The chunks itself are linked lists of rows, which simplifies space reuse.

Every `Table` block consists of:

| Size (B)  | Offset (B)    | Type           | Usage
|-          |-              |-               |-
| 1         | 0             | `u8`           | How many rows is saved in this block. Used for selecting block to insert rows in.
| Variable  | 1             | `RowSpec[255]` | 255 rows.

`RowSpec` format:
| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 8         | 0             | `HeapPtr`     | Location of the next row.
| 1         | 8             | `u8`          | 1 if row is used, 0 otherwise
| Variable  | 9             | `Row`         | A row itself.

`Row` is a packed array of `Value`s depending on table's columns. This means that when a column is added/dropped/changed type, all rows must be re-added.

### Data
This is a data heap, saved using the [free store](https://github.com/sppmacd/heap) implementation.

Every block consists of:

| Size (B)  | Offset (B)    | Type           | Usage
|-          |-              |-               |-
| 1         | 0             |                | TODO

## Value format

### `ValueType`
Values for `ValueType` correspond to the [Value::Type](../db/core/Value.hpp#L18) enum:
* `0x00` - Null
* `0x01` - Int
* `0x02` - Float
* `0x03` - Varchar
* `0x04` - Bool
* `0x05` - Time

### `DynamicValue`
| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 1         | 0             | `ValueType`   | Value type
| 16        | 0             | `Value`       | The value itself

### `Value`
Value depending on its [type](#valuetype) is:

* For Null: nothing
* For Int: a `i32 LE` (4B)
* For Float: a `f32` (4B)
* For Varchar: a `HeapSpan` encoding a UTF-8 string (16B)
* For Bool: a `bool` (1B)
* For Time: `u16` year + `u8` month + `u8` day (4B)

If value can be NULL, it is prepended by a *is null* `bool`. If *is null* is true, the actually stored value is undefined.

The DynamicValue is always 14B regardless of its type.
