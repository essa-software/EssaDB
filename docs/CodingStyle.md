## Coding Style

We want to keep the EssaDB engine source code as clean and readable as possible. We take some patterns from [SerenityOS AK](https://github.com/SerenityOS/serenity/tree/master/AK), like `TRY()` and `is<T>()` dynamic cast helper.

### Low-level Formatting
- Format code according to the `.clang-format` file placed in a project root (in VSCode, you can press `Ctrl+Shift+I` to format code automatically)

### Name styles
- Try to name everything as accurately as possible.
- Use full words for names (no unnecessary shortcuts).
- Use following naming conventions:
    - Functions, variables, namespaces and methods: `snake_case`
    - Classes, enumerations, enumeration members, constants: `BigCamelCase`
    - Macros: `ALL_CAPS` (but generally avoid them)

### Variables
- Add prefixes to variables:
    - `m_` to private fields.
    - `g_` to globals (but avoid them except in very specific case).
    - `s_` to all kinds of statics
- Don't add any prefixes to another kinds of symbols (functions, methods, classes etc.)
- Generally, don't make member variables public. Use getters and setters instead.
- Add `set_` to setters. Add `get_` to getters if they are not free (e.g do some kind of map lookup); otherwise, just use member name (without `m_`) 
- Mandatory arguments should be passed by reference (not by pointer).

### Classes and namespaces
- For types with methods, prefer `class` over `struct`.
- Use structs when you want to use designated initializers (see [example definition](https://github.com/essa-software/EssaDB/blob/077b83b6b8b3fd93b41c81874b450f0dd3a4c19f/db/core/Select.hpp#L55) and its [usage](https://github.com/essa-software/EssaDB/blob/077b83b6b8b3fd93b41c81874b450f0dd3a4c19f/db/sql/Parser.cpp#L342)).
- Use constructor initializer lists if possible (`MyClass(int a) : m_a(a) {}`).
- Use `enum class` instead of just `enum` in most cases.
- Don't `use` the whole namespace (e.g. `using namespace std`). Use `using` for specific objects (e.g. `using std::cout`) if this would improve readability, otherwise just spell out the whole name (`std::cout << "something";`)

### Framework modules / libraries
- The libraries should be created in a separate folder, named the same as that folder.
- When you create a new library (or header), ensure that a global header that link all other headers in a directory exists.
    - This global header should be named the same as a folder which it links.

### Headers and source files
- Use `#pragma once` for include guards.
- Name headers the same as class names.
- `#include` headers using `<class>` syntax when including files from another module (e.g `core` from `sql`), and `"class"` when including files that are in the same directory. Beware of the strange clangd behavior which includes headers like `#include "db/core/AST.hpp"`.
- Use `.hpp` extension for header file and `.cpp` for source files.

Proper header file layout:

```c++
// (Optional) copyright header, file version, other general information

#pragma once

#include "..."
#include <...>

namespace foo {

class Bar {
    // ...
};

}
```

Proper source file layout:

```c++
// (Optional) copyright header, file version, other general information

#include "CorrespondingHeader.hpp"

#include "..."
#include <...>

namespace foo {

// ...

}
```

### Expressions/Statements
- Use range-for (`for (auto& member: c)`)
- Use `\033` for ESC in escape sequences.

### Comments
- Make comments look like sentences.
- Explain *why* the code is doing something, not *what* it is doing.

## Types
- Use east const (`MyClass const&`)
