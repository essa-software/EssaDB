add_executable(essadb-gui
    main.cpp
)
essautil_setup_target(essadb-gui)
target_link_libraries(essadb-gui essadb EssaGUI::EssaGUI)
