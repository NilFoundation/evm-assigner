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

# TODO: link to blueprint and crypto3 here after fixing this in crypto3:
# https://github.com/NilFoundation/crypto3/issues/175
target_link_libraries(${PROJECT_NAME} INTERFACE
                      evmone)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/nil/blueprint/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nil/blueprint)
#evmc
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../evmc/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#evmone
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/vm.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/execution_state.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/tracing.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/baseline.hpp
              ${CMAKE_CURRENT_SOURCE_DIR}/../evmone/eof.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_ASSIGNER_TESTS)
    add_subdirectory(test)
endif()
