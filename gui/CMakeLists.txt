set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(MySQL)

add_executable(essadb-gui
    DatabaseModel.cpp
    main.cpp
)
essautil_setup_target(essadb-gui)
target_include_directories(essadb-gui PRIVATE ${MYSQL_INCLUDE_DIR})
target_link_libraries(essadb-gui essadb EssaGUI::EssaGUI ${MYSQL_LIBRARY})
