add_executable(essadb-repl
    main.cpp
)
essautil_setup_target(essadb-repl)
target_link_libraries(essadb-repl essadb)
