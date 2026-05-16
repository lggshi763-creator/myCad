# Unit-test target. Real tests will be added in Sprint 0.1 (Catch2 hello world)
# and grown out as each layer's domain logic lands.

add_executable(mycad_unit_tests
    domain/shared/hello_test.cpp
    domain/shared/value_objects_test.cpp
    domain/shared/identity_test.cpp
    domain/shared/domain_event_test.cpp
    domain/shared/geometry_port_test.cpp
    domain/shared/ports_test.cpp
    domain/shared/geometry_construction_port_test.cpp
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

# ---------------------------------------------------------------------------
# Infrastructure integration tests — separate target because it links OCCT.
# Keep apart from mycad_unit_tests so the domain-layer purity check (V4) can
# grep mycad_unit_tests sources without finding any OCCT symbols.
# ---------------------------------------------------------------------------
add_executable(mycad_infra_tests
    infrastructure/geometry/occt_adapter_test.cpp
    infrastructure/eventsourcing/in_memory_event_store_test.cpp
    infrastructure/eventsourcing/sqlite_eventstore_test.cpp
    infrastructure/ecs/entt_registry_test.cpp
    infrastructure/persistence/mycad_file_store_test.cpp
    application/createbox_test.cpp
)

target_link_libraries(mycad_infra_tests
    PRIVATE
        Catch2::Catch2WithMain
        mycad::infrastructure
        mycad::application
        mycad::compile_options
)

catch_discover_tests(mycad_infra_tests
    REPORTER junit
    OUTPUT_DIR ${MYCAD_TEST_REPORTS_DIR}
    OUTPUT_SUFFIX .xml
)

# ---------------------------------------------------------------------------
# Application layer tests — CommandBus, command handlers, integration.
# Separate from mycad_unit_tests so domain-purity check (V6) stays clean.
# ---------------------------------------------------------------------------
add_executable(mycad_app_tests
    application/commandbus_test.cpp
)

target_link_libraries(mycad_app_tests
    PRIVATE
        Catch2::Catch2WithMain
        mycad::application
        mycad::compile_options
)

catch_discover_tests(mycad_app_tests
    REPORTER junit
    OUTPUT_DIR ${MYCAD_TEST_REPORTS_DIR}
    OUTPUT_SUFFIX .xml
)
