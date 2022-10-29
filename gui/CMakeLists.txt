cmake_minimum_required(VERSION 3.0)
project(hello-mysql-world C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

include(FindPkgConfig)
pkg_check_modules(LIBMYSQLCLIENT REQUIRED mysqlclient)

foreach(FLAG ${LIBMYSQLCLIENT_CFLAGS_OTHER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
endforeach()

link_directories(${LIBMYSQLCLIENT_LIBRARY_DIRS})

add_executable(essadb-gui
    DatabaseModel.cpp
    main.cpp
)
essautil_setup_target(essadb-gui)
target_include_directories(essadb-gui PRIVATE ${LIBMYSQLCLIENT_INCLUDE_DIRS})
target_link_libraries(essadb-gui essadb EssaGUI::EssaGUI ${LIBMYSQLCLIENT_LIBRARIES})
