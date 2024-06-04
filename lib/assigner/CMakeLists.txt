cmake_minimum_required(VERSION 2.8.12)

cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0063 NEW)

option(BUILD_ASSIGNER_TESTS "Build unit tests" FALSE)

add_library(${PROJECT_NAME} STATIC src/vm_host.cpp src/handler_base.cpp)

find_package(crypto3 REQUIRED)
find_package(blueprint_crypto3 REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../evmone>)

target_link_libraries(${PROJECT_NAME}
                      PUBLIC evmone crypto3::all blueprint
                      PRIVATE ethash::keccak)

# Install assigner headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# Install library
install(TARGETS ${PROJECT_NAME} EXPORT assignerTargets DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install evmone headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/vm.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/execution_state.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/tracing.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/baseline.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/eof.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/instructions_opcodes.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_ASSIGNER_TESTS)
    add_subdirectory(test)
endif()
