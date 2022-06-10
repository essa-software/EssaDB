function(add_test test_name)
    add_executable(test-${test_name}
        setup.cpp
        testcases/${test_name}.cpp
    )
    setup_target(test-${test_name})
    target_link_libraries(test-${test_name} essadb)
endfunction()

add_test(csv)
add_test(execution)
add_test(where)
