// Benchmarks of uniform_disc() functions.
#include <benchmark/benchmark.h>

#include "random.h"
#include "ray.h"

namespace {

void BM_UniformDisc1(benchmark::State& state) {
  Random rng;
  for (auto _ : state) {
    benchmark::DoNotOptimize(vec2::uniform_disc(rng));
  }
}
BENCHMARK(BM_UniformDisc1);

void BM_UniformDisc2(benchmark::State& state) {
  Random rng;
  for (auto _ : state) {
    benchmark::DoNotOptimize(vec2::uniform_disc2(rng));
  }
}
BENCHMARK(BM_UniformDisc2);

void BM_UniformDisc3(benchmark::State& state) {
  Random rng;
  for (auto _ : state) {
    benchmark::DoNotOptimize(vec2::uniform_disc3(rng));
  }
}
BENCHMARK(BM_UniformDisc3);

}  // namespace

BENCHMARK_MAIN();
