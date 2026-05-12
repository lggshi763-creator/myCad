# Unit-test target. Real tests will be added in Sprint 0.1 (Catch2 hello world)
# and grown out as each layer's domain logic lands.

add_executable(mycad_unit_tests
    domain/shared/hello_test.cpp
)

target_link_libraries(mycad_unit_tests
    PRIVATE
        Catch2::Catch2WithMain
        mycad::domain
        mycad::compile_options
)

# JUnit reports go here for CI artifact upload. `catch_discover_tests` does NOT
# auto-create OUTPUT_DIR — Catch2 just tries to open the file for writing and
# fails with "Unable to open file" if the directory doesn't exist. So we
# create it at configure time.
set(MYCAD_TEST_REPORTS_DIR ${CMAKE_BINARY_DIR}/test-reports)
file(MAKE_DIRECTORY ${MYCAD_TEST_REPORTS_DIR})

catch_discover_tests(mycad_unit_tests
    REPORTER junit
    OUTPUT_DIR ${MYCAD_TEST_REPORTS_DIR}
    # Use .xml extension on report files; matches what most CI test parsers expect.
    OUTPUT_SUFFIX .xml
)
