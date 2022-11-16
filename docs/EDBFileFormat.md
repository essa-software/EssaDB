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
The header is of size 4096 B.

Header structure

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              | -
| 6         | 0             |               | Filemagic (`esdb\r\n` / `65 73 64 62 0d 0a`).
| 2         | 6             | `u16 LE`      | File version. This document describes version `0x0000`.
| 4         | 12            | `HeapIndex`   | Index of first block of `Table`.
| 4         | 16            | `HeapIndex`   | Index of first block of `DataAllocInfo`.
| 4076      | 20            | zeros         | Reserved.

The first block is always a [`TableStructure`](#tablestructure) block.

### Heap
The heap consists of **blocks**, each 4096 B (4 KiB) in size so that it fits one VM page.

Blocks form **regions**, which are abstracted in the API so that they can be read contiguously. That said, descriptions in [Block types](#block-types) section don not take blocks into account.

Indexes to the heap are specified by `HeapIndex`, which is a little-endian u32 specifying index of heap block, starting from 1. The value of 0 means no block. The actual offset in the file is *index* * 4096.

Indexes to the `Data` region are specified by `DataPtr`, which is 6B in size and consists of:

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 4         | 0             | `HeapIndex`   | Block containing the data
| 2         | 4             | `HeapIndex`   | Offset onto the heap block

Contiguous data is specified by `DataSpan`, which consists of `DataPtr` and `u64` specifying size. Its is 14B in size.

Block structure:

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 4         | 0             | `HeapIndex`   | Pointer to the next block. `0` if this is the end.
| 1         | 1             | `bool`        | True if block is unused. Set when block is freed and is used for block allocation.
| 4091      | 5             |               | Actual data.

## Region types
* `TableStructure` - stores columns
* `Table` - stores rows
* `Data` - stores values of dynamically sized attributes, like varchars, and any other dynamic data, in a form of [free store](https://github.com/sppmacd/heap).

### TableStructure

`TableStructure` store all data global to a table: columns, row count, keys, constraints, checks, auto-increment variables etc.

Format:

| Size (B)  | Offset (B)        | Type          | Usage
|-          |-                  |-              |-
| 8         | 0                 | `u64 LE`      | Number of rows
| 1         | 8                 | `u8`          | Number of columns (`Col`)
| 14        | 9                 | `DataSpan`    | Table name
| 14        | 23                | `DataSpan`    | Check SQL statement
| 1         | 37                | `u8`          | Number of auto-increment variables (`Aiv`)
| 1         | 38                | `u8`          | Number of keys (`Key`)
| 8         | 39                | `u64 LE`      | Index of last known free row chunk, used for optimizing inserts.
| `Col`     | 47                | `Col[]`       | Column definitions
| `Aiv`     | 47+`Col`          | `Aiv[]`       | Auto-increment variable definitions
| `Key`     | 47+`Col`+`Aiv`    | `Key[]`       | Keys

Column format (`Col`):

| Size (B)  | Offset (B)    | Type                              | Usage
|-          |-              |-                                  |-
| 14        | 0             | `DataSpan`                        | Column name
| 1         | 14            | [`ValueType`](#valuetype)         | Value type
| 1         | 15            | `bool`                            | Auto increment
| 1         | 16            | `bool`                            | Is unique
| 1         | 17            | `bool`                            | Is non null
| 14        | 18            | [`DynamicValue`](#dynamicvalue)   | Default value

Auto-increment variable format (`Aiv`):

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 1         | 0             | `u8`          | Column index
| 8         | 1             | `u64 LE`      | Value

Key format (`Key`):

| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 1         | 0             | `KeyType`     | Key type
| 1         | 1             | `u8`          | Local column
| 14        | 2             | `DataSpan`    | References table (for FOREIGN KEY)
| 1         | 16            | `u8`          | References column (for FOREIGN KEY)

Key types:
* `0x00` - PRIMARY
* `0x01` - FOREIGN

### Table

`Table` contains actual rows, grouped into chunks of 256. It allows for reasonably fast searching for free space to insert new blocks. The chunks itself are linked lists of rows, which simplifies space reuse.
 
Every `Table` block consists of:

| Size (B)  | Offset (B)    | Type           | Usage
|-          |-              |-               |-
| 2         | 0             | `u16 LE`       | How much rows can yet fit here. Used for selecting block to insert rows in.
| Variable  | 2             | `RowSpec[256]` | 256 rows.
| Variable  | Variable      | zeros          | Padding.

`RowSpec` format:
| Size (B)  | Offset (B)    | Type          | Usage
|-          |-              |-              |-
| 4         | 0             | `ChunkPtr`    | Location of the next row.
| Variable  | 4             | `Row`         | A row itself.

`Row` is a packed array of `Value`s depending on table's columns. This means that when a column is added/dropped/changed type, all rows must be re-added.

### Data
This is a data heap, saved using the [free store](https://github.com/sppmacd/heap) implementation.

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
| 14        | 0             | `Value`       | The value itself

### `Value`
Value depending on its [type](#valuetype) is:

* For Null: nothing
* For Int: a `i32 LE` (4B)
* For Float: a `f32` (4B)
* For Varchar: a `DataSpan` encoding a UTF-8 string (14B)
* For Bool: a `bool` (1B)
* For Time: `u16` year + `u8` month + `u8` day (4B)

If value can be NULL, it is prepended by a *is null* `bool`. If *is null* is true, the actually stored value is undefined.

The DynamicValue is always 14B regardless of its type.
