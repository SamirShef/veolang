cmake_minimum_required(VERSION 3.20)
project(veoc LANGUAGES CXX C VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)
if(NOT LLVM_VERSION_MAJOR EQUAL 21)
    message(FATAL_ERROR "LLVM version required 21.x.y, but found ${LLVM_PACKAGE_VERSION}")
endif()
message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} was found")
message(STATUS "LLVM path: ${LLVM_DIR}")

if("LLVM" IN_LIST LLVM_AVAILABLE_LIBS)
    set(llvm_libs LLVM)
else()
    set(llvm_libs LLVM_AVAILABLE_LIBS)
endif()

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_libraries(${LLVM_LIBRARY_DIRS})
message(STATUS "Libs ${llvm_libs}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${llvm_libs})

if(LLVM_SYSTEM_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_SYSTEM_LIBS})
endif()
