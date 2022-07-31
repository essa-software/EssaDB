project(EssaDB)
cmake_minimum_required(VERSION 3.17)

find_package(EssaGUI REQUIRED)

add_subdirectory(db)
add_subdirectory(gui)
add_subdirectory(repl)
add_subdirectory(tests)
