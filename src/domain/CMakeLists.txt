add_library(mycad_domain STATIC placeholder.cpp)
add_library(mycad::domain ALIAS mycad_domain)

target_compile_features(mycad_domain PUBLIC cxx_std_20)
target_include_directories(mycad_domain PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(mycad_domain PRIVATE mycad::compile_options)
