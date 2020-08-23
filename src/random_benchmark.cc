// Benchmarks of random number generator.
#include <benchmark/benchmark.h>

#include "random.h"

namespace {

// Squares: A Fast Counter-Based RNG
// https://arxiv.org/abs/2004.06278
uint32_t squares(uint64_t ctr, uint64_t key) {
  uint64_t x, y, z;
  y = x = ctr * key;
  z = y + key;
  x = x * x + y;
  x = (x >> 32) | (x << 32); /* round 1 */
  x = x * x + z;
  x = (x >> 32) | (x << 32); /* round 2 */
  return (x * x + y) >> 32;
}

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

void BM_Squares(benchmark::State& state) {
  uint64_t key = 0x1234567812345678;
  uint64_t ctr = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(squares(ctr++, key));
  }
}
BENCHMARK(BM_Squares);

}  // namespace

BENCHMARK_MAIN();
