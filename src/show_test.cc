// Example of show.cc.
#include <cstring>
#include <cmath>
#include <memory>

#include "show.h"

int main() {
  const int w = 640;
  const int h = 480;
  std::unique_ptr<uint8_t[]> data(new uint8_t[w * h * 4]);

  auto putpixel = [&data](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t* ptr = data.get() + 4 * (y * w + x);
    ptr[0] = b;
    ptr[1] = g;
    ptr[2] = r;
  };

  memset(data.get(), 0, w * h * 4);

  for (int j = 100; j < 200; j++)
    for (int i = 100; i < 200; i++) {
      putpixel(i, j, 127, 127, 127);

      uint8_t v = .5 + 255. * pow(.5, 1./2.2);
      putpixel(i + 200, j, v, v, v);
    }

  const int border = 20;
  for (int j = 100 + border; j < 200 - border; j++)
    for (int i = 100 + border; i < 200 - border; i++) {
      uint8_t v = ((i ^ j) & 1) ? 255 : 0;
      putpixel(i, j, v, v, v);
      putpixel(i + 200, j, v, v, v);
    }

  for (int j = 0; j < 20; j++)
    for (int i = 0; i < 100; i++) {
      putpixel(i, j, 255, 0, 0);
      putpixel(i + 100, j, 0, 255, 0);
      putpixel(i + 200, j, 0, 0, 255);
    }

  show(w, h, data.get());
}
