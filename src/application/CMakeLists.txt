add_library(mycad_application STATIC placeholder.cpp)
add_library(mycad::application ALIAS mycad_application)

target_compile_features(mycad_application PUBLIC cxx_std_20)
target_include_directories(mycad_application PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(mycad_application
    PUBLIC  mycad::domain
    PRIVATE mycad::compile_options
)
