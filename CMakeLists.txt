cmake_minimum_required(VERSION 3.20)
project(veoc LANGUAGES CXX C VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
if(NOT LLVM_VERSION_MAJOR EQUAL 18)
    message(FATAL_ERROR "LLVM version required 18.x.y, but found ${LLVM_PACKAGE_VERSION}")
endif()
message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} was found")
message(STATUS "LLVM path: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS all)

if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_options(/GR- /EHs-c-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-fno-rtti -fno-exceptions)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})
