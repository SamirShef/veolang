add_executable(test_parser
    main.cpp
    test_var_def.cpp
)

target_link_libraries(test_parser PRIVATE veoc-core)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(test_parser PRIVATE
    PARSER_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/"
)

add_test(NAME Subsystem.Parser COMMAND test_parser)
