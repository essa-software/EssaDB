## Coding Style

### Low-level Formatting
- Format code according to the `.clang-format` file placed in a project root (you can press `Ctrl+Shift+I` if you use VS Code to format code automatically)

### Name styles
- Try to name everything as accurately as possible.
- Use full words for names (no unnecessary shortcuts).
- Functions, variables, namespaces and methods: `snake_case`
- Classes, enumerations, enumeration members: `BigCamelCase`
- Macros: `ALL_CAPS` (but generally avoid them)

### Variables
- Add prefixes to variables:
    - `m_` to private variables (don't add anything to methods).
    - `g_` to globals (but avoid them except in very specific case).
    - `s_` to all kinds of statics, except these function-local.
- Generally, don't make member variables public. Use getters and setters instead.
- Add `set_` to setters. Add `get_` to getters if they are not free (e.g does some kind of map lookup), otherwise just use member name (without `m_`)
- Mandatory arguments should be passed by reference (not by pointer).

### Classes and namespaces
- For types with methods, prefer `class` over `struct`.
- Use structs when you want to use designated initializers (see [example](https://github.com/essa-software/EssaDB/blob/077b83b6b8b3fd93b41c81874b450f0dd3a4c19f/db/core/Select.hpp#L55)).
- Use constructor initializers (`MyClass class(int a) : m_a(a) {}`).
- Prefer `enum class` over plain `enum`.
- Don't `use` the whole namespace (e.g. `using namespace std`). Use `using` for specific objects (e.g. `using std::cout`).

### Framework modules / libraries
- The libraries should be created in a separate folder, named the same as that folder.
- When you create a new library (or header), ensure that a global header that link all other headers in a directory exists.
    - This global header should be named the same as a folder which it links.

### Headers
- Use `#pragma once` for include guards.
- Name headers the same as class names.
- `#include` headers using `<class>` syntax when including files from another module (e.g `core` from `sql`), and `"class"` when including files that are in the same directory. Beware of the strange clangd behavior which includes headers like `#include "db/core/AST.hpp"`.

### Expressions/Statements
- Use range-for (`for(auto& member: c)`)
- `\033` for ESC used in escape sequences.

### Comments
- Make comments look like sentences.
- Explain *why* the code is doing something, not *what* it is doing.

## Types
- Use east const (`MyClass const&`)
