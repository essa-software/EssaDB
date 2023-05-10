set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(MySQL)

essa_executable(essadb-gui
    LIBS
    essadb
    Essa::GUI
    ${MYSQL_LIBRARY}

    SOURCES
    client/ConnectToEssaDBDialog.cpp
    client/ConnectToMySQLDialog.cpp
    client/DatabaseClient.cpp
    client/EssaDBDatabaseClient.cpp
    client/MySQLDatabaseClient.cpp
    ConnectDialog.cpp
    DatabaseModel.cpp
    main.cpp
    SQLSyntaxHighlighter.cpp
)
target_include_directories(essadb-gui PRIVATE ${MYSQL_INCLUDE_DIR})
essa_resources(essadb-gui assets)
