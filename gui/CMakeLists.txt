set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(MySQL)

add_executable(essadb-gui
client/ConnectToMySQLDialog.cpp
    client/DatabaseClient.cpp
    client/EssaDBDatabaseClient.cpp
    client/MySQLDatabaseClient.cpp
    ConnectDialog.cpp
    DatabaseModel.cpp
    main.cpp
)
essautil_setup_target(essadb-gui)
target_include_directories(essadb-gui PRIVATE ${MYSQL_INCLUDE_DIR})
target_link_libraries(essadb-gui essadb EssaGUI::EssaGUI ${MYSQL_LIBRARY})
