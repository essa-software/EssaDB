add_executable(essadb-gui
    main.cpp
)
setup_target(essadb-gui)
target_link_libraries(essadb-gui essadb essa-gui)
