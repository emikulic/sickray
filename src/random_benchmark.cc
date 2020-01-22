// Benchmarks of uniform_disc() functions.
#include <benchmark/benchmark.h>

#include "random.h"

namespace {

void BM_Construct(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(Random());
  }
}
BENCHMARK(BM_Construct);

void BM_Next(benchmark::State& state) {
  Random rng;
  for (auto _ : state) {
    benchmark::DoNotOptimize(rng.next());
  }
}
BENCHMARK(BM_Next);

void BM_Rand(benchmark::State& state) {
  Random rng;
  for (auto _ : state) {
    benchmark::DoNotOptimize(rng.rand());
  }
}
BENCHMARK(BM_Rand);

void BM_Fork(benchmark::State& state) {
  Random rng;
  for (auto _ : state) {
    rng = rng.fork(1);
    benchmark::DoNotOptimize(rng);
  }
}
BENCHMARK(BM_Fork);

}  // namespace

BENCHMARK_MAIN();
