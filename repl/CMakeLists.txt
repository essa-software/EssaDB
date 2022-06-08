add_executable(essadb-repl
    main.cpp
)
setup_target(essadb-repl)
target_link_libraries(essadb-repl essadb)
