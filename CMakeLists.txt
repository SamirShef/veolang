cmake_minimum_required(VERSION 3.20)
project(veoc LANGUAGES CXX C)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} was found")
message(STATUS "LLVM path: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

if(NOT LLVM_ENABLE_RTTI)
    if(MSVC)
        add_compile_options(/GR-)
    else()
        add_compile_options(-fno-rtti)
    endif()
endif()
if(NOT LLVM_ENABLE_EH)
    if(MSVC)
        add_compile_options(/EHs- /EHc-)
    else()
        add_compile_options(-fno-exceptions)
    endif()
endif()

add_compile_options(-fno-rtti -fno-exceptions -g)

if(TARGET LLVM)
    set(LLVM_LINK_LIBS LLVM)
    message(STATUS "Using shared LLVM target")
else()
    llvm_map_components_to_libnames(
        LLVM_LINK_LIBS
        core
        support
        native
        analysis
        target
        mc
        bitreader
        passes
    )
    message(STATUS "Using static LLVM components: ${LLVM_LINK_LIBS}")
endif()

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*/main\\.cpp$|main\\.cpp$")

add_library(veoc-core STATIC ${SOURCES})
target_link_libraries(veoc-core PRIVATE
    tomlplusplus::tomlplusplus
    ${LLVM_LINK_LIBS}
    ${LLVM_SYSTEM_LIBS}
)
target_include_directories(veoc-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(
    ${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE veoc-core)

enable_testing()

add_executable(
    test_lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/lexer/main.cpp
)
add_test(
    NAME TestLexer
    COMMAND test_lexer ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/lexer
)
target_compile_options(test_lexer PRIVATE -O0 -g)
target_link_libraries(test_lexer PRIVATE veoc-core)

add_executable(
    test_parser
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/parser/main.cpp
)
add_test(
    NAME TestParser
    COMMAND test_parser ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/parser
)
target_compile_options(test_parser PRIVATE -O0 -g)
target_link_libraries(test_parser PRIVATE veoc-core)
