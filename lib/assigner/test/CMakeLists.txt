hunter_add_package(GTest)

include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(assigner_tests assigner_test.cpp)

target_link_libraries(
        assigner_tests
        PRIVATE
        ${PROJECT_NAME}
        GTest::gtest_main
)
add_test(NAME assigner_tests COMMAND assigner_tests)
set_tests_properties(assigner_tests PROPERTIES LABELS ASSIGNER)

add_custom_target(run_assigner_tests COMMAND ${CMAKE_CTEST_COMMAND} -L ASSIGNER --output-on-failure WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(run_assigner_tests assigner_tests)
