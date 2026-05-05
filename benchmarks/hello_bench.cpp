#include <mycad/domain/Hello.hpp>

#include <benchmark/benchmark.h>

#include <string>

static void BM_Greet(benchmark::State& state) {
    const std::string name(state.range(0), 'a');
    for (auto _ : state) {
        auto result = mycad::domain::greet(name);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Greet)->Arg(8)->Arg(64)->Arg(256);

static void BM_Greet_Unicode(benchmark::State& state) {
    for (auto _ : state) {
        auto result = mycad::domain::greet("世界");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Greet_Unicode);

BENCHMARK_MAIN();
