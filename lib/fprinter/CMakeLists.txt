include(GoogleTest)

add_custom_target(fprinter DEPENDS fprinter.h)

add_executable(fprinter-tests fprinter_test.cpp)

target_link_libraries(
        fprinter-tests
        PRIVATE
        GTest::gtest_main
)
add_dependencies(fprinter-tests fprinter)
