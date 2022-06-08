add_executable("test-essadb"
    setup.cpp
    execution.cpp
)
setup_target("test-essadb")
target_link_libraries("test-essadb" essadb)
