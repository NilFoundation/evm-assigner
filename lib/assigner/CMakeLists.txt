cmake_minimum_required(VERSION 2.8.12)

cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0063 NEW)

option(BUILD_ASSIGNER_TESTS "Build unit tests" FALSE)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
                           ${CMAKE_CURRENT_SOURCE_DIR}/include
                           ${include_dir}/evmone
                           ${CMAKE_CURRENT_SOURCE_DIR}/../evmone

                           ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} INTERFACE
                      crypto3::blueprint
                      evmone)

if(BUILD_ASSIGNER_TESTS)
    add_subdirectory(test)
endif()
