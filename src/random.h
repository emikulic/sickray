#pragma once
#include <cstdint>
#include <cstring>

class Random {
 public:
  Random() = delete;  // Unsafe.

  // Don't initialize with all zeros.
  Random(uint64_t a, uint64_t b, uint64_t c, uint64_t d, int mix_steps = 11)
      : s{a, b, c, d} {
    // Need ~11 calls to start mixing.
    for (int i = 0; i < mix_steps; ++i) {
      next();
    }
  }

  // Returns a random number in the range [0, 1)
  double rand() {
    // Generate a random number in the range [1, 2).
    // It will have the form: 0x3FFn nnnn nnnn nnnn
    uint64_t out = next();
    out &= 0x000FFFFFFFFFFFFFULL;
    out |= 0x3FF0000000000000ULL;
    double d;
    memcpy(&d, &out, 8);
    return d - 1.;
  }

  // Random number generator using xoshiro256+ from:
  // http://xoshiro.di.unimi.it/xoshiro256plus.c
  uint64_t next(void) {
    const uint64_t result_plus = s[0] + s[3];

    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = rotl(s[3], 45);

    return result_plus;
  }

  static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

  uint64_t s[4];
};
