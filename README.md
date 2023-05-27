# EssaDB - ESSA Database
Welcome to ESSA Database Engine! Our goal is to be better than MS Access (which is not that hard). EssaDB is going to have fully implemented SQL 
dialect (called EssaSQL obviously, right?). We want to be as efective as it's possible including future great optimizations to Database Engine and our 
file format being [clever implementation of data heap](docs/EDBFileFormat.md).

## How to build our code?

You will require:
- `cmake` and `ninja` for building
- `mysql` for MySQL support

For the most basic development setup:

1. Create and go to a build directory:
```sh
mkdir build
cd build
```

2. Configure project with CMake:
```sh
cmake .. -GNinja
```

3. Build
```sh
ninja
```

4. Run
```sh
./repl/essadb-repl
./gui/essadb-gui
```

For release / production, add `-DCMAKE_BUILD_TYPE=Release` to CMake command (step 2). Note that running as before [won't work](https://github.com/essa-software/EssaGUI/blob/main/docs/Packaging.md#build-types), you need to install:

```sh
ninja install

# By default it outputs executables in /usr/local/bin:
$ which essadb-repl
/usr/local/bin/essadb-repl

# Run as normal command if you have /usr/local/bin in PATH:
essadb-repl
essadb-gui
```

## Directory structure

* *db* - stores compiled EssaDB Engine
* *gui* - EssaDB with GUI
* *repl* - EssaDB console
* *tests* - plenty of unit tests showing real capabilities of EssaSQL

We are going to make docs folder for our SQL variant in the future, feel free to use, discuss and contribute!

## How to contribute?

* See [contributing guidelines](./CONTRIBUTING.md) first.
