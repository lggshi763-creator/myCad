add_executable(mycad_benchmarks
    hello_bench.cpp
)

target_link_libraries(mycad_benchmarks
    PRIVATE
        benchmark::benchmark
        mycad::domain
        mycad::compile_options
)
