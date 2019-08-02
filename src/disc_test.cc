// Example of uniform_disc().
#include <memory>

#include "random.h"
#include "ray.h"
#include "show.h"

int main() {
  constexpr int sz = 800;
  std::unique_ptr<uint8_t[]> data(new uint8_t[sz * sz * 4]);

  auto putpixel = [&data](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t* ptr = data.get() + 4 * (y * sz + x);
    ptr[0] = b;
    ptr[1] = g;
    ptr[2] = r;
  };

  memset(data.get(), 0, sz * sz * 4);

  Random rng(0, 0, 0, 1);
  for (int i = 0; i < 1000; ++i) {
    vec2 v = (vec2::uniform_disc(rng) / 2. * .9 + vec2{.5, .5}) * sz;
    putpixel(v.x, v.y, 255, 255, 255);
  }
  Show(sz, sz, data.get());
}
