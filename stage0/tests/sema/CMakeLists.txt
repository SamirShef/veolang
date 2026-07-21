add_executable(test_sema main.cpp)

target_compile_options(test_sema PRIVATE -g)
target_link_libraries(test_sema PRIVATE veoc-core)
target_include_directories(test_sema PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(test_sema PRIVATE
    SEMA_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/"
)

add_test(NAME Subsystem.Sema COMMAND test_sema)
