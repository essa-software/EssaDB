project(EssaDB)
cmake_minimum_required(VERSION 3.17)

find_package(EssaGUI REQUIRED)

# GCC moment
add_compile_options(-Wno-error=restrict)

add_subdirectory(db)
add_subdirectory(gui)
add_subdirectory(repl)
add_subdirectory(tests)
