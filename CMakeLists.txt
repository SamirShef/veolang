cmake_minimum_required(VERSION 3.20)
project(veoc LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} was found")
message(STATUS "LLVM path: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
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

add_compile_options(-fno-rtti -fno-exceptions)

if(TARGET LLVM)
    set(LLVM_LINK_LIBS LLVM)
    message(STATUS "Using shared LLVM target")
else()
    llvm_map_components_to_libnames(LLVM_LINK_LIBS
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LINK_LIBS} ${LLVM_SYSTEM_LIBS})

# if(NOT LLVM_LIBRARIES)
#     llvm_map_components_to_libnames(llvm_libs_fallback ${llvm_components})
#     target_link_libraries(${PROJECT_NAME} PRIVATE ${llvm_libs_fallback})
#
#     message(STATUS "Linking with llvm_libs (fallback): ${llvm_libs_fallback}")
# else()
#     message(STATUS "Linking with llvm_libs: ${LLVM_LIBRARIES}")
# endif()
