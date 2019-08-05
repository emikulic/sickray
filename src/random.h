#pragma once
#include <cstdint>
#include <cstring>
#include <limits>

class Random {
 public:
  Random() {
    s[0] = mix(1, 1);
    s[1] = mix(2, 1);
    s[2] = mix(3, 1);
    s[3] = mix(4, 1);
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

  // Returns a new rng with the new mixin added.
  Random fork(uint64_t mixin) const {
    Random out = *this;
    out.s[0] = mix(out.s[0], mixin);
    out.s[1] = mix(out.s[1], out.s[0]);
    out.s[2] = mix(out.s[2], out.s[1]);
    out.s[3] = mix(out.s[3], out.s[2]);

    out.s[2] = mix(out.s[2], out.s[3]);
    out.s[1] = mix(out.s[1], out.s[2]);
    out.s[0] = mix(out.s[0], out.s[1]);
    out.next();
    return out;
  }

  static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

  static uint64_t mix(uint64_t state, uint64_t mixin) {
    // A multiplier that has been found to provide good mixing.
    constexpr uint64_t kMul = 0xdc3eb94af8ab4c93ULL;
    state *= kMul;
    state = ((state << 19) |
             (state >> (std::numeric_limits<uint64_t>::digits - 19))) +
            mixin;
    return state;
  }

  uint64_t s[4];
};
