project(EssaDB)
cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 20)

function(setup_target targetname)
    install(TARGETS ${targetname} EXPORT EssaGUI DESTINATION lib)

    target_compile_options(${targetname} PRIVATE -Wall -Wextra -Werror)
    target_compile_options(${targetname} PRIVATE
        -Wnon-virtual-dtor
        -Wno-error=missing-field-initializers       # FIXME: This is buggy (?) for *DrawOptions
        -Wno-error=format                           # FIXME: We don't use formatting anyway except SimulationClock but this breaks mingw build
    )

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_options(${targetname} PUBLIC -fsanitize=undefined,address)
        target_link_options(${targetname} PUBLIC -fsanitize=undefined,address)
    else()
        target_compile_options(${targetname} PUBLIC -O3)
    endif()
    target_include_directories(${targetname} PUBLIC ${CMAKE_SOURCE_DIR}/src)
endfunction()

add_executable(
    essadb

    src/core/Database.cpp
    src/core/RowWithColumnNames.cpp
    src/core/SelectResult.cpp
    src/core/Table.cpp
    src/core/Value.cpp

    src/main.cpp
)

setup_target(essadb)

