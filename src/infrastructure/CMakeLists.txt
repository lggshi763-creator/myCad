add_library(mycad_infrastructure STATIC placeholder.cpp)
add_library(mycad::infrastructure ALIAS mycad_infrastructure)

target_compile_features(mycad_infrastructure PUBLIC cxx_std_20)
target_include_directories(mycad_infrastructure PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(mycad_infrastructure
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
)
