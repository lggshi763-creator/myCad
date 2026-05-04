# Unit-test target. Real tests will be added in Sprint 0.1 (Catch2 hello world)
# and grown out as each layer's domain logic lands.

add_executable(mycad_unit_tests placeholder_test.cpp)

target_link_libraries(mycad_unit_tests
    PRIVATE
        Catch2::Catch2WithMain
        mycad::compile_options
)

catch_discover_tests(mycad_unit_tests
    REPORTER junit
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-reports
)
