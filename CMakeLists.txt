cmake_minimum_required(VERSION 3.17)
project(EssaDB)

include(FetchContent)
FetchContent_Declare(Essa
    GIT_REPOSITORY https://github.com/essa-software/EssaGUI.git
    GIT_TAG f53265bb4eff3c6d030eabadc2c774d9355cd508
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(Essa)

# GCC moment
add_compile_options(-Wno-error=restrict)

include_directories(${Essa_SOURCE_DIR}) # FIXME: This should be automatic
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(db)
add_subdirectory(gui)
add_subdirectory(repl)
add_subdirectory(tests)
