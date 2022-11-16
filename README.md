# EssaDB - ESSA Database
Welcome to ESSA Database Engine! Our goal is to be better than MS Access (which is not that hard). EssaDB is going to have fully implemented SQL 
dialect (called EssaSQL obviously, right?). We want to be as efective as it's possible including future great optimizations to Database Engine and our 
file format being [clever implementation of data heap](docs/EDBFileFormat.md).

## How to build our code?

To build EssaDB to your local machine, you will need CMake tool and some project generator (we prefer Ninja to work with Essa Products). Clone this repository and EssaGUI (dependency for EssaDB): https://github.com/essa-software/EssaGUI.

You will also need MySQL C++ Connector for MySQL Database connection support. You can get it from: https://github.com/mysql/mysql-connector-cpp

Follow installation instructions for EssaGUI, then prepare a workspace for yourself (see commands below, run it in EssaDB root folder):

```
mkdir build
cd build
cmake -GNinja ..
ninja
```

Let your generator with CMake take care of the rest. After successful compilation, several new folders should occur:

```
ls .
CMakeCache.txt  CMakeFiles  build.ninja  cmake_install.cmake  compile_commands.json  db  gui  repl  tests
```

* *db* - stores compiled EssaDB Engine
* *gui* - EssaDB with GUI
* *repl* - EssaDB console
* *tests* - plenty of unit tests showing real capabilities of EssaSQL

We are going to make docs folder for our SQL variant in the future, feel free to use, discuss and contribute!

## How to contribute?

* See [contributing guidelines](./CONTRIBUTING.md) first.
