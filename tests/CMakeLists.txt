function(add_test test_name)
    add_executable(test-${test_name}
        setup.cpp
        testcases/${test_name}.cpp
    )
    setup_target(test-${test_name})
    target_link_libraries(test-${test_name} essadb)
endfunction()

add_test(arithmetic)
add_test(csv)
add_test(db_manipulation)
add_test(execution)
add_test(multiple)
add_test(numeric_func)
add_test(sql)
