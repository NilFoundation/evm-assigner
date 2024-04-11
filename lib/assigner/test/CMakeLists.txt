include(GoogleTest)

add_executable(assigner_tests assigner_test.cpp)

#target_include_directories(assigner_tests PRIVATE
#                           ${CMAKE_CURRENT_SOURCE_DIR}/../include
#                           ${CMAKE_CURRENT_SOURCE_DIR}/../../blueprint/include
#                           ${CMAKE_CURRENT_SOURCE_DIR}/../../crypto3/libs/)

target_link_libraries(
        assigner_tests
        PRIVATE
        ${PROJECT_NAME}
        GTest::gtest_main
)

